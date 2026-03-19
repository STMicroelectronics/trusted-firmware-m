/*
 * Copyright (c) 2026, STMicroelectronics. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <string.h>
#include <lib/utils_def.h>
#include <psa/crypto.h>
#include <psa/error.h>
#include <tfm_sp_log.h>
#include <bootutil_priv.h>
#include <bootutil/bootutil.h>
#include <bootutil/image.h>
#include <flash_map_backend/flash_map_backend.h>
#include <sysflash/sysflash.h>
#include <tfm_bootloader_fwu_abstraction.h>
#include <tfm_boot_status.h>
#include <service_api.h>

#if (FWU_COMPONENT_NUMBER != MCUBOOT_IMAGE_NUMBER)
	#error "FWU_COMPONENT_NUMBER mismatch with MCUBOOT_IMAGE_NUMBER"
#endif

#define MAX_IMAGE_INFO_LENGTH   (MCUBOOT_IMAGE_NUMBER * \
				 (sizeof(struct image_version) + \
				 SHARED_DATA_ENTRY_HEADER_SIZE))

/*
 * \struct fwu_image_info_data
 *
 * \brief Contains the received boot status information from bootloader
 *
 * \details This is a redefinition of \ref tfm_boot_data to allocate the
 *	    appropriate, service dependent size of \ref boot_data.
 */
typedef struct fwu_image_info_data_s {
	struct shared_data_tlv_header header;
	uint8_t data[MAX_IMAGE_INFO_LENGTH];
} fwu_image_info_data_t;

typedef struct tfm_fwu_mcuboot_ctx_s {
	/* The flash area corresponding to component. */
	const struct flash_area *fap;

	/* The size of the downloaded data in the FWU process. */
	size_t loaded_size;
} tfm_fwu_mcuboot_ctx_t;

static __maybe_unused tfm_fwu_mcuboot_ctx_t mcuboot_ctx[FWU_COMPONENT_NUMBER];
static fwu_image_info_data_t __aligned(4) boot_shared_data;

static psa_status_t fwu_bootloader_get_shared_data(void)
{
	return tfm_core_get_boot_data(TLV_MAJOR_FWU,
				      (struct tfm_boot_data *)&boot_shared_data,
				      sizeof(boot_shared_data));
}

/**
 * Get the active slot id.
 *
 * Return the version that was launched by mcuboot thanks to shared data.
 */
static psa_status_t get_active_image_version(psa_fwu_component_t component,
					     struct image_version *image_ver)
{
	struct shared_data_tlv_entry tlv_entry;
	uint8_t *tlv_end;
	uint8_t *tlv_curr;

	/* The bootloader writes the image version information into the memory which
	 * is shared between MCUboot and TF-M. Read the shared memory.
	 */
	if (boot_shared_data.header.tlv_magic != SHARED_DATA_TLV_INFO_MAGIC)
		return PSA_ERROR_DATA_CORRUPT;

	tlv_end = (uint8_t *)&boot_shared_data +
			     boot_shared_data.header.tlv_tot_len;
	tlv_curr = boot_shared_data.data;

	while (tlv_curr < tlv_end) {
		(void)memcpy(&tlv_entry, tlv_curr, SHARED_DATA_ENTRY_HEADER_SIZE);
		if ((GET_FWU_CLAIM(tlv_entry.tlv_type) == SW_VERSION) &&
		    (GET_FWU_MODULE(tlv_entry.tlv_type) == component)) {
			if (tlv_entry.tlv_len != sizeof(struct image_version)) {
				return PSA_ERROR_DATA_CORRUPT;
			}
			memcpy(image_ver,
			       tlv_curr + SHARED_DATA_ENTRY_HEADER_SIZE,
			       tlv_entry.tlv_len);
			return PSA_SUCCESS;
		}
		tlv_curr += SHARED_DATA_ENTRY_HEADER_SIZE + tlv_entry.tlv_len;
	}

	return PSA_ERROR_DATA_CORRUPT;
}

psa_status_t fwu_bootloader_init(void)
{
	if (fwu_bootloader_get_shared_data() != PSA_SUCCESS)
		return PSA_ERROR_STORAGE_FAILURE;

#if defined(STM32_FWU_STORAGE_ACCESS)
	/* add Init of specific flash driver */
	flash_area_driver_init();
#endif
	return PSA_SUCCESS;
}

#if defined(STM32_FWU_STORAGE_ACCESS)
/**
 * The following 5 functions are adaptations of the standard mcuboot shim
 * layer to support RAM load. The main differences between the two files
 * are in this section.
 */

/**
 * Compare image version numbers, not including the build number.
 *
 * @v1: First image version to compare.
 * @v2: Second image version to compare.
 *
 * Return:
 * true  - if v1 is greater or equal to v2
 * false - if v1 is less than v2
 */
static bool is_version_greater_or_equal(const struct image_version *v1,
					const struct image_version *v2)
{
	if (v1->iv_major != v2->iv_major)
		return v1->iv_major > v2->iv_major;

	if (v1->iv_minor != v2->iv_minor)
		return v1->iv_minor > v2->iv_minor;

	return v1->iv_revision >= v2->iv_revision;
}

/**
 * Copy of mcuboot boot_image_load_header without error messages.
 *
 * @fa_p: Flash area of the image slot.
 * @hdr:  Returned image header.
 *
 * Return:
 * 0              - on success
 * BOOT_EFLASH    - on flash access error
 * BOOT_EBADIMAGE - if the header is invalid
 */
static int image_load_header(const struct flash_area *fa_p,
			     struct image_header *hdr)
{
	uint32_t size;
	int rc = flash_area_read(fa_p, 0, hdr, sizeof(*hdr));

	if (rc != 0)
		return BOOT_EFLASH;

	if (hdr->ih_magic != IMAGE_MAGIC || (hdr->ih_flags & IMAGE_F_NON_BOOTABLE))
		return BOOT_EBADIMAGE;

	if (!boot_u32_safe_add(&size, hdr->ih_img_size, hdr->ih_hdr_size) ||
	    size >= flash_area_get_size(fa_p))
		return BOOT_EBADIMAGE;

	return 0;
}

/**
 * Get slot id according to active / secondary semantics.
 *
 * Definitions:
 * - "Valid" slot:
 *      slot with a good magic and a valid header.
 *
 * - Active slot:
 *      slot on which TFM has booted. It is the valid slot with the
 *      highest image version. image_ok is not used for this decision.
 *
 * - Primary slot:
 *      among valid slots only:
 *        * if exactly one has image_ok == BOOT_FLAG_SET, that slot is primary.
 *        * if both or none have image_ok == BOOT_FLAG_SET, the primary slot
 *          is the valid slot with the highest image version.
 *
 * - Secondary slot:
 *      the other slot with respect to the primary slot.
 *
 * @component: FWU component identifier.
 * @active:    true  -> return the active slot id
 *             false -> return the secondary slot id
 * @id:        returned slot id.
 *
 * Return:
 * PSA_SUCCESS                  - on success
 * PSA_ERROR_INVALID_ARGUMENT   - if @id is NULL
 * PSA_ERROR_STORAGE_FAILURE    - on flash access / metadata errors
 * PSA_ERROR_NOT_PERMITTED      - on impossible cases
 */
static psa_status_t ram_load_get_slot_id(psa_fwu_component_t component,
					 bool active, uint8_t *id)
{
	const struct flash_area *fap_a = NULL, *fap_b = NULL;
	const uint8_t id_a = FLASH_AREA_IMAGE_PRIMARY(component);
	const uint8_t id_b = FLASH_AREA_IMAGE_SECONDARY(component);
	struct boot_swap_state state_a = { 0 }, state_b = { 0 };
	struct image_header hdr_a = { 0 }, hdr_b = { 0 };
	bool a_valid = false, b_valid = false;
	bool a_ge_b = false;
	psa_status_t ret = PSA_SUCCESS;
	int err;

	if (!id)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (flash_area_open(id_a, &fap_a) != 0) {
		LOG_ERRFMT("TFM FWU: slot a: opening flash failed.\r\n");
		return PSA_ERROR_STORAGE_FAILURE;
	}

	if (flash_area_open(id_b, &fap_b) != 0) {
		LOG_ERRFMT("TFM FWU: slot b: opening flash failed.\r\n");
		flash_area_close(fap_a);
		return PSA_ERROR_STORAGE_FAILURE;
	}

	if (boot_read_swap_state(fap_a, &state_a) != 0 ||
	    boot_read_swap_state(fap_b, &state_b) != 0) {
		ret = PSA_ERROR_STORAGE_FAILURE;
		goto close_return;
	}

	/*
	 * Load headers.
	 *
	 * - BOOT_EBADIMAGE means the slot is not a valid image.
	 * - BOOT_EFLASH is treated as a storage error.
	 */
	err = image_load_header(fap_a, &hdr_a);
	if (err == 0) {
		a_valid = (state_a.magic == BOOT_MAGIC_GOOD);
	} else if (err == BOOT_EFLASH) {
		ret = PSA_ERROR_STORAGE_FAILURE;
		goto close_return;
	}

	err = image_load_header(fap_b, &hdr_b);
	if (err == 0) {
		b_valid = (state_b.magic == BOOT_MAGIC_GOOD);
	} else if (err == BOOT_EFLASH) {
		ret = PSA_ERROR_STORAGE_FAILURE;
		goto close_return;
	}

	/* If both slots are valid, pre-compute which one has the highest version. */
	if (a_valid && b_valid)
		a_ge_b = is_version_greater_or_equal(&hdr_a.ih_ver, &hdr_b.ih_ver);

	if (active) {
		/*
		 * Active slot:
		 * - among valid slots only,
		 * - slot with the highest image version.
		 */
		if (a_valid && !b_valid) {
			*id = id_a;
		} else if (!a_valid && b_valid) {
			*id = id_b;
		} else if (a_valid && b_valid) {
			*id = a_ge_b ? id_a : id_b;
		} else {
			/* No valid slot: inconsistent state. */
			ret = PSA_ERROR_NOT_PERMITTED;
		}
		goto close_return;
	}

	/*
	 * Secondary slot detection:
	 * - derived from the primary slot definition,
	 *   but only considering valid slots.
	 *
	 * Primary slot among valid slots:
	 *   1) If exactly one slot has image_ok == BOOT_FLAG_SET,
	 *      that slot is primary.
	 *   2) Otherwise (both or none), primary is the valid slot
	 *      with the highest image version.
	 * Secondary slot is the other one.
	 */

	/* No valid slot at all: inconsistent state. */
	if (!a_valid && !b_valid) {
		ret = PSA_ERROR_NOT_PERMITTED;
		goto close_return;
	}

	/* Only one valid slot. The valid slot is the primary one, the other is secondary. */
	if (a_valid && !b_valid) {
		*id = id_b;
		goto close_return;
	}

	if (!a_valid && b_valid) {
		*id = id_a;
		goto close_return;
	}

	/* Here, both slots are valid. Check image_ok first. */

	if (state_a.image_ok == BOOT_FLAG_SET &&
	    state_b.image_ok != BOOT_FLAG_SET) {
		/* primary = a, secondary = b */
		*id = id_b;
		goto close_return;
	}

	if (state_b.image_ok == BOOT_FLAG_SET &&
	    state_a.image_ok != BOOT_FLAG_SET) {
		/* primary = b, secondary = a */
		*id = id_a;
		goto close_return;
	}

	/*
	 * Both or none have image_ok == SET.
	 * Primary = slot with highest version.
	 * Secondary = the other.
	 */
	*id = a_ge_b ? id_b : id_a;

close_return:
	flash_area_close(fap_a);
	flash_area_close(fap_b);

	return ret;
}

/**
 * Get the active slot id.
 *
 * Active slot is the slot on which TFM has booted.
 * Note: active slot != primary slot.
 */
static psa_status_t get_active_slot(psa_fwu_component_t component, uint8_t *id)
{
	return ram_load_get_slot_id(component, true, id);
}

/**
 * Get the secondary slot id.
 *
 * Secondary slot is the slot that can be used to store the image to update.
 */
static psa_status_t get_secondary_slot(psa_fwu_component_t component, uint8_t *id)
{
	return ram_load_get_slot_id(component, false, id);
}

psa_status_t fwu_bootloader_staging_area_init(psa_fwu_component_t component,
					      const void *manifest,
					      size_t manifest_size)
{
	const struct flash_area *fap;
	psa_status_t ret;
	uint8_t image_id;

	/* MCUboot uses bundled manifest. */
	if ((manifest_size != 0) || (component >= FWU_COMPONENT_NUMBER))
		return PSA_ERROR_INVALID_ARGUMENT;

	ret = get_secondary_slot(component, &image_id);
	if (ret) {
		LOG_ERRFMT("Unable to get image slot.\r\n");
		return ret;
	}

	if (flash_area_open(image_id, &fap) != 0) {
		LOG_ERRFMT("TFM FWU: opening flash failed.\r\n");
		return PSA_ERROR_STORAGE_FAILURE;
	}

	if (flash_area_erase(fap, 0, fap->fa_size) != 0) {
		LOG_ERRFMT("TFM FWU: erasing flash failed.\r\n");
		return PSA_ERROR_GENERIC_ERROR;
	}

	mcuboot_ctx[component].fap = fap;

	/* Reset the loaded_size. */
	mcuboot_ctx[component].loaded_size = 0;

	return PSA_SUCCESS;
}

psa_status_t fwu_bootloader_load_image(psa_fwu_component_t component,
				       size_t block_offset,
				       const void *block,
				       size_t block_size)
{
	const struct flash_area *fap;

	if (block == NULL || component >= FWU_COMPONENT_NUMBER)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* The component should already be added into the mcuboot_ctx. */
	if (mcuboot_ctx[component].fap != NULL) {
		fap = mcuboot_ctx[component].fap;
	} else {
		return PSA_ERROR_BAD_STATE;
	}

	if (flash_area_write(fap, block_offset, block, block_size) != 0) {
		LOG_ERRFMT("TFM FWU: write flash failed.\r\n");
		return PSA_ERROR_STORAGE_FAILURE;
	}

	/* The overflow check has been done in flash_area_write. */
	mcuboot_ctx[component].loaded_size += block_size;
	return PSA_SUCCESS;
}

static int set_pending_multi(int image_index, int permanent)
{
	const struct flash_area *fap;
	int rc;
	uint8_t image_id;

	if (get_secondary_slot(image_index, &image_id)) {
		LOG_ERRFMT("%s: Unable to get image_index slot.\r\n", __func__);
		return BOOT_ENOMEM;
	}

	rc = flash_area_open(image_id, &fap);
	if (rc != 0)
		return BOOT_EFLASH;

	rc = boot_set_next(fap, false, !(permanent == 0));

	flash_area_close(fap);
	return rc;
}

/* The classic TLV iterator cannot be used in RAM-load mode because it reads
 * data from RAM, while in this case TLVs must be read directly from storage.
 */
#if defined(MCUBOOT_RAM_LOAD)
/*
 * Initialize a TLV iterator.
 *
 * @param it An iterator struct
 * @param hdr image_header of the slot's image
 * @param fap flash_area of the slot which is storing the image
 * @param type Type of TLV to look for
 * @param prot true if TLV has to be stored in the protected area, false otherwise
 *
 * @returns 0 if the TLV iterator was successfully started
 *          -1 on errors
 */
static int
tfm_fwu_tlv_iter_begin(struct image_tlv_iter *it, const struct image_header *hdr,
		       const struct flash_area *fap, uint16_t type, bool prot)
{
	uint32_t offset;
	struct image_tlv_info info;

	if (it == NULL || hdr == NULL || fap == NULL)
		return -1;

	offset = hdr->ih_hdr_size + hdr->ih_img_size;

	if (flash_area_read(fap, offset, &info, sizeof(info)))
		return -1;

	if (info.it_magic == IMAGE_TLV_PROT_INFO_MAGIC) {
		if (hdr->ih_protect_tlv_size != info.it_tlv_tot)
			return -1;

		if (flash_area_read(fap, offset + info.it_tlv_tot, &info,
				    sizeof(info)))
			return -1;
	} else if (hdr->ih_protect_tlv_size != 0) {
		return -1;
	}

	if (info.it_magic != IMAGE_TLV_INFO_MAGIC)
		return -1;

	it->hdr = hdr;
	it->fap = fap;
	it->type = type;
	it->prot = prot;
	it->prot_end = offset + it->hdr->ih_protect_tlv_size;
	it->tlv_end = offset + it->hdr->ih_protect_tlv_size + info.it_tlv_tot;

	/* Position on first TLV entry (right after the TLV info header). */
	it->tlv_off = offset + sizeof(info);

	return 0;
}

/*
 * Find next TLV
 *
 * @param it The image TLV iterator struct
 * @param off The offset of the TLV's payload in flash
 * @param len The length of the TLV's payload
 * @param type If not NULL returns the type of TLV found
 *
 * @returns 0 if a TLV with matching type was found
 *          1 if no more TLVs with matching type are available
 *          -1 on errors
 */
static int
tfm_fwu_tlv_iter_next(struct image_tlv_iter *it, uint32_t *off, uint16_t *len,
		      uint16_t *type)
{
	struct image_tlv tlv;
	int rc;

	if (it == NULL || it->hdr == NULL || it->fap == NULL)
		return -1;

	while (it->tlv_off < it->tlv_end) {
		if (it->hdr->ih_protect_tlv_size > 0 &&
		    it->tlv_off == it->prot_end)
			it->tlv_off += sizeof(struct image_tlv_info);

		rc = flash_area_read(it->fap, it->tlv_off, &tlv, sizeof(tlv));
		if (rc)
			return -1;

		/* No more TLVs in the protected area */
		if (it->prot && it->tlv_off >= it->prot_end)
			return 1;

		if (it->type == IMAGE_TLV_ANY || tlv.it_type == it->type) {
			if (type != NULL)
				*type = tlv.it_type;

			*off = it->tlv_off + sizeof(tlv);
			*len = tlv.it_len;

			it->tlv_off += sizeof(tlv) + tlv.it_len;
			return 0;
		}

		it->tlv_off += sizeof(tlv) + tlv.it_len;
	}

	return 1;
}
#else
static int
tfm_fwu_tlv_iter_begin(struct image_tlv_iter *it, const struct image_header *hdr,
		       const struct flash_area *fap, uint16_t type, bool prot)
{
	return bootutil_tlv_iter_begin(it, hdr, fap, type, prot);
}

static int
tfm_fwu_tlv_iter_next(struct image_tlv_iter *it, uint32_t *off, uint16_t *len,
		      uint16_t *type)
{
	return bootutil_tlv_iter_next(it, off, len, type);
}
#endif

psa_status_t fwu_bootloader_install_image(const psa_fwu_component_t *candidates, uint8_t number)
{
	uint8_t index_i, cand_index;
#if (MCUBOOT_IMAGE_NUMBER > 1)
	psa_fwu_component_t component;
	const struct flash_area *fap;
	struct image_tlv_iter it;
	struct image_header hdr;
	int rc;
	uint32_t off;
	uint16_t len;
	struct image_dependency dep;
	struct image_version image_ver = { 0 };
	const struct flash_area *fap_secondary;
	struct image_header hdr_secondary;
	bool check_pass = true;
#endif

	if (candidates == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

#if (MCUBOOT_IMAGE_NUMBER > 1)
	for (cand_index = 0; cand_index < number; cand_index++) {
		component = candidates[cand_index];
		/* The image should already be added into the mcuboot_ctx. */
		if ((component >= FWU_COMPONENT_NUMBER) ||
		    (mcuboot_ctx[component].fap == NULL))
			return PSA_ERROR_INVALID_ARGUMENT;

		fap = mcuboot_ctx[component].fap;
		/* Read the image header. */
		if (flash_area_read(fap, 0, &hdr, sizeof(hdr)) != 0)
			return PSA_ERROR_STORAGE_FAILURE;

		/* Return PSA_ERROR_DATA_CORRUPT if the image header is invalid. */
		if (hdr.ih_magic != IMAGE_MAGIC)
			return PSA_ERROR_DATA_CORRUPT;

		/* Initialize the iterator. */
		if (tfm_fwu_tlv_iter_begin(&it, &hdr, fap, IMAGE_TLV_DEPENDENCY, true))
			return PSA_ERROR_STORAGE_FAILURE;

		/* Check dependencies. */
		while (true) {
			rc = tfm_fwu_tlv_iter_next(&it, &off, &len, NULL);
			if (rc < 0) {
				return PSA_ERROR_STORAGE_FAILURE;
			} else if (rc > 0) {
				/* No more dependency found. */
				rc = 0;
				break;
			}

			/* Check against payload length overflow */
			if (len > sizeof(dep)) {
				return PSA_ERROR_INVALID_ARGUMENT;
			}

			/* A dependency requirement is found. Set check_pass to false. */
			check_pass = false;
			if (flash_area_read(fap, off, &dep, len) != 0)
				return PSA_ERROR_STORAGE_FAILURE;

			if (dep.image_id > MCUBOOT_IMAGE_NUMBER)
				return PSA_ERROR_DATA_CORRUPT;

			/* As this partition does not validate the image in the secondary slot,
			 * so it has no information of which image will be chosen to run after
			 * reboot. So if the dependency image in the primary slot or that in the
			 * secondary slot can meet the dependency requirement, then the
			 * dependency check pass.
			 */
			/* Check the dependency image in the primary slot. */
			if (get_active_image_version(dep.image_id,
						     &image_ver) != PSA_SUCCESS)
				return PSA_ERROR_STORAGE_FAILURE;

			/* Check whether the version of the running image can meet the
			 * dependency requirement.
			 */
			if (is_version_greater_or_equal(&image_ver,
							&dep.image_min_version)) {
				check_pass = true;
			} else {
				/* Check whether the CANDIDATE image can meet this image's
				 * dependency requirement.
				 */
				for (index_i = 0; index_i < number; index_i++) {
					if (candidates[index_i] == dep.image_id)
						break;
				}

				if ((index_i < number) && (mcuboot_ctx[dep.image_id].fap != NULL)) {
					/* The running image cannot meet the dependency requirement.
					 * Check the dependency image in the secondary slot.
					 */
					fap_secondary = mcuboot_ctx[dep.image_id].fap;
					if (flash_area_read(fap_secondary,
							    0,
							    &hdr_secondary,
							    sizeof(hdr_secondary)) != 0)
						return PSA_ERROR_STORAGE_FAILURE;

					/* Check the version of the dependency image in the
					 * secondary slot only if the image header is good.
					 */
					if (hdr_secondary.ih_magic == IMAGE_MAGIC &&
					    (is_version_greater_or_equal(&hdr_secondary.ih_ver,
									 &dep.image_min_version)))
						/* The dependency image in the secondary slot meet
						 * the dependency requirement.
						 */
						check_pass = true;
				}
			}

			/* Return directly if dependency check fails. */
			if (!check_pass)
				return PSA_ERROR_DEPENDENCY_NEEDED;
		}
	}
#endif

	/* Write the boot magic in image trailer so that these images will be
	 * taken as candidates.
	 */
	for (cand_index = 0; cand_index < number; cand_index++) {
		if (set_pending_multi(candidates[cand_index], false) != 0) {
			/* If failure happens, reject candidates already installed. */
			for (index_i = 0; index_i < cand_index; index_i++) {
				if (fwu_bootloader_reject_staged_image(candidates[index_i])
				    != PSA_SUCCESS)
					break;
			}
			return PSA_ERROR_STORAGE_FAILURE;
		}
	}
	return PSA_SUCCESS_REBOOT;
}

static inline uint32_t boot_magic_off(const struct flash_area *fap)
{
	return flash_area_get_size(fap) - BOOT_MAGIC_SZ;
}

#if (defined(MCUBOOT_RAM_LOAD) && defined(MCUBOOT_RAM_LOAD_REVERT))
static inline uint32_t boot_image_ok_off(const struct flash_area *fap)
{
	return ALIGN_DOWN(boot_magic_off(fap) - BOOT_MAX_ALIGN, BOOT_MAX_ALIGN);
}

static psa_status_t erase_image_ok(const struct flash_area *fap)
{
	uint32_t off;
	uint8_t buf[BOOT_MAX_ALIGN];
	uint8_t erased_val;
	uint32_t align;

	/* off of image OK flag is already BOOT_MAX_ALIGN aligned. */
	off = boot_image_ok_off(fap);

	/* Clear the image ok trailer. */
	align = flash_area_align(fap);
	align = ALIGN_UP(BOOT_MAX_ALIGN, align);
	if (align > BOOT_MAX_ALIGN)
		return PSA_ERROR_STORAGE_FAILURE;

	erased_val = flash_area_erased_val(fap);
	memset(buf, erased_val, align);
	if (flash_area_write(fap, off, buf, align) != 0)
		return PSA_ERROR_STORAGE_FAILURE;

	return PSA_SUCCESS;
}

static int set_confirmed_multi(int image_index)
{
	const struct flash_area *fap = NULL;
	int rc;
	uint8_t image_id;

	if (get_active_slot(image_index, &image_id)) {
		LOG_ERRFMT("%s: Unable to get image_index slot.\r\n", __func__);
		return BOOT_ENOMEM;
	}

	rc = flash_area_open(image_id, &fap);
	if (rc != 0) {
		return BOOT_EFLASH;
	}

	rc = boot_set_next(fap, true, true);

	flash_area_close(fap);
	return rc;
}
#endif

psa_status_t fwu_bootloader_mark_image_accepted(const psa_fwu_component_t *trials,
						uint8_t number)
{
	/* Image revert is supported in RAM_LOAD upgrade strategy when
	 * MCUBOOT_RAM_LOAD_REVERT is true. In these cases, the image needs to
	 * be set as a permanent image explicitly. Then the accepted image can
	 * still be selected as the running image during next time reboot up.
	 * Otherwise, the image will be reverted and the previous one will be
	 * chosen as the running image.
	 */
#if (defined(MCUBOOT_RAM_LOAD) && defined(MCUBOOT_RAM_LOAD_REVERT))
	uint8_t trial_index, i, image_id;
	psa_fwu_component_t component;
	const struct flash_area *fap;
	psa_status_t ret;

	if (trials == NULL)
		return PSA_ERROR_INVALID_ARGUMENT;

	for (trial_index = 0; trial_index < number; trial_index++) {
		component = trials[trial_index];
		if (component >= FWU_COMPONENT_NUMBER)
			return PSA_ERROR_INVALID_ARGUMENT;

		ret = get_secondary_slot(component, &image_id);
		if (ret) {
			LOG_ERRFMT("Unable to get component %u image slot.\r\n", component);
			return ret;
		}

		if (flash_area_open(image_id, &fap) != 0)
			return PSA_ERROR_STORAGE_FAILURE;

		if (fap == NULL)
			return PSA_ERROR_INVALID_ARGUMENT;

		mcuboot_ctx[component].fap = fap;
	}
	for (trial_index = 0; trial_index < number; trial_index++) {
		component = trials[trial_index];
		if (set_confirmed_multi(component) != 0) {
			for (i = 0; i < trial_index; i++) {
				if (erase_image_ok(mcuboot_ctx[component].fap) != 0) {
					break;
				}
			}
			return PSA_ERROR_STORAGE_FAILURE;
		}
	}
#else
	(void)trials;
	(void)number;
#endif
	return PSA_SUCCESS;
}

static psa_status_t erase_boot_magic(const struct flash_area *fap)
{
	uint32_t off, pad_off;
	uint8_t magic[BOOT_MAGIC_ALIGN_SIZE];
	uint8_t erased_val;

	off = boot_magic_off(fap);
	pad_off = ALIGN_DOWN(off, BOOT_MAX_ALIGN);
	erased_val = flash_area_erased_val(fap);
	memset(&magic[0], erased_val, sizeof(magic));

	/* Clear the boot magic trailer. */
	if (flash_area_write(fap, pad_off, &magic[0], BOOT_MAGIC_ALIGN_SIZE))
		return PSA_ERROR_STORAGE_FAILURE;

	return PSA_SUCCESS;
}

/* Reject the staged image. */
psa_status_t fwu_bootloader_reject_staged_image(psa_fwu_component_t component)
{
	if (component >= FWU_COMPONENT_NUMBER)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* The image should already be added into the mcuboot_ctx. */
	if (mcuboot_ctx[component].fap != NULL)
		return erase_boot_magic(mcuboot_ctx[component].fap);

	/* The component is not in FWU process. */
	return PSA_ERROR_DOES_NOT_EXIST;
}

/* Reject the running image in trial state. */
psa_status_t fwu_bootloader_reject_trial_image(psa_fwu_component_t component)
{
	if (component >= FWU_COMPONENT_NUMBER)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* The image will be reverted if it is not accepted explicitly. */
	return PSA_SUCCESS_REBOOT;
}

psa_status_t fwu_bootloader_clean_component(psa_fwu_component_t component)
{
	const struct flash_area *fap = NULL;

	if (component >= FWU_COMPONENT_NUMBER)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* Check if the image is in a FWU process. */
	if (mcuboot_ctx[component].fap != NULL) {
		fap = mcuboot_ctx[component].fap;
		if (flash_area_erase(fap, 0, fap->fa_size) != 0)
			return PSA_ERROR_STORAGE_FAILURE;

		/* close flash aera to match ready state */
		flash_area_close(fap);
		mcuboot_ctx[component].fap = NULL;
	} else {
		return PSA_ERROR_DOES_NOT_EXIST;
	}

	return PSA_SUCCESS;
}

static psa_status_t util_img_hash(const struct flash_area *fap,
				  size_t data_size,
				  uint8_t *hash_result,
				  size_t buf_size,
				  size_t *hash_size)
{
	psa_hash_operation_t handle = psa_hash_operation_init();
	psa_status_t status;
	uint8_t tmpbuf[BOOT_TMPBUF_SZ];
	uint32_t tmp_buf_sz = BOOT_TMPBUF_SZ;
	uint32_t blk_sz;
	uint32_t off;

	/* Setup the hash object for the desired hash. */
	status = psa_hash_setup(&handle, PSA_ALG_SHA_256);
	if (status != PSA_SUCCESS)
		return status;

	for (off = 0; off < data_size; off += blk_sz) {
		blk_sz = data_size - off;
		if (blk_sz > tmp_buf_sz)
			blk_sz = tmp_buf_sz;

		if (flash_area_read(fap, off, tmpbuf, blk_sz))
			return PSA_ERROR_STORAGE_FAILURE;

		status = psa_hash_update(&handle, tmpbuf, blk_sz);
		if (status != PSA_SUCCESS)
			return status;
	}

	status = psa_hash_finish(&handle, hash_result, buf_size, hash_size);

	return status;
}

static psa_status_t get_second_image_digest(psa_fwu_component_t component,
					    psa_fwu_component_info_t *info)
{
	const struct flash_area *fap = NULL;
	uint8_t hash[TFM_FWU_MAX_DIGEST_SIZE] = {0};
	size_t hash_size = 0;
	psa_status_t ret = PSA_SUCCESS;
	size_t data_size;
	uint8_t image_id;

	if (component >= FWU_COMPONENT_NUMBER)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* Check if the image is in a FWU process. */
	if (mcuboot_ctx[component].fap != NULL) {
		/* Calculate hash on the downloaded data. */
		data_size = mcuboot_ctx[component].loaded_size;
	} else {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (get_secondary_slot(component, &image_id) != 0) {
		LOG_ERRFMT("Unable to get component %u image slot.\r\n", component);
		return ret;
	}

	if ((flash_area_open(image_id, &fap)) != 0) {
		LOG_ERRFMT("TFM FWU: opening flash failed.\r\n");
		return PSA_ERROR_STORAGE_FAILURE;
	}

	if (util_img_hash(fap, data_size, hash, (size_t)TFM_FWU_MAX_DIGEST_SIZE,
			  &hash_size) == PSA_SUCCESS) {
		memcpy(info->impl.candidate_digest, hash, hash_size);
	} else {
		ret = PSA_ERROR_STORAGE_FAILURE;
	}

	flash_area_close(fap);
	return ret;
}
#else /* defined(STM32_FWU_STORAGE_ACCESS) */

psa_status_t fwu_bootloader_staging_area_init(psa_fwu_component_t component,
					      const void *manifest,
					      size_t manifest_size)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t fwu_bootloader_reject_staged_image(psa_fwu_component_t component)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t fwu_bootloader_load_image(psa_fwu_component_t component,
				       size_t block_offset,
				       const void *block,
				       size_t block_size)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t fwu_bootloader_install_image(const psa_fwu_component_t *candidates, uint8_t number)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t fwu_bootloader_mark_image_accepted(const psa_fwu_component_t *trials,
						uint8_t number)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t fwu_bootloader_reject_trial_image(psa_fwu_component_t component)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t fwu_bootloader_clean_component(psa_fwu_component_t component)
{
	return PSA_ERROR_NOT_SUPPORTED;
}
#endif /* defined(STM32_FWU_STORAGE_ACCESS) */

psa_status_t fwu_bootloader_get_image_info(psa_fwu_component_t component,
					   bool query_state,
					   bool query_impl_info,
					   psa_fwu_component_info_t *info)
{
	uint8_t __maybe_unused image_ok = BOOT_FLAG_UNSET;
	const struct flash_area __maybe_unused *fap = NULL;
	struct image_version image_version;
	psa_status_t ret = PSA_SUCCESS;
	uint8_t __maybe_unused image_id;

	if (info == NULL)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (component >= FWU_COMPONENT_NUMBER)
		return PSA_ERROR_INVALID_ARGUMENT;

#if defined(STM32_FWU_STORAGE_ACCESS)
	/* During the FWU sequence, the active slot matches the primary slot
	 * until the trial state. Then, the active slot becomes the secondary
	 * slot. Once the update is accepted, the secondary slot becomes the
	 * new primary slot.
	 */
	ret = get_active_slot(component, &image_id);
	if (ret) {
		LOG_ERRFMT("Unable to get component %u secondary slot.\r\n", component);
		return ret;
	}

	if ((flash_area_open(image_id, &fap)) != 0) {
		LOG_ERRFMT("TFM FWU: opening flash failed.\r\n");
		return PSA_ERROR_STORAGE_FAILURE;
	}
	info->max_size = fap->fa_size;
	info->location = fap->fa_id;
	info->flags = PSA_FWU_FLAG_VOLATILE_STAGING;

	if (query_state) {
		/* Get value of image-ok flag of the image to check whether application
		 * itself is already confirmed.
		 */
		if (boot_read_image_ok(fap, &image_ok) != 0) {
			ret = PSA_ERROR_STORAGE_FAILURE;
			goto close_return;
		}

		if (image_ok == BOOT_FLAG_SET) {
			info->state = PSA_FWU_READY;
		} else {
			info->state = PSA_FWU_TRIAL;
		}
	}
#else
	/* Default values, they are meaningless when STM32_FWU_STORAGE_ACCESS is OFF */
	info->max_size = 0U;
	info->location = 0U;
	info->flags = PSA_FWU_FLAG_VOLATILE_STAGING;
	info->state = PSA_FWU_READY;
#endif

	if (get_active_image_version(component, &image_version) == PSA_SUCCESS) {
		info->version.major = image_version.iv_major;
		info->version.minor = image_version.iv_minor;
		info->version.patch = image_version.iv_revision;
		info->version.build = image_version.iv_build_num;
	} else {
		ret = PSA_ERROR_STORAGE_FAILURE;
		goto close_return;
	}
#if defined(STM32_FWU_STORAGE_ACCESS)
	if (query_impl_info)
		ret = get_second_image_digest(component, info);
#endif

close_return:
#if defined(STM32_FWU_STORAGE_ACCESS)
	flash_area_close(fap);
#endif
	return ret;
}
