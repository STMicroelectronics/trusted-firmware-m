/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmsis_compiler.h>

#include <device.h>
#include <debug.h>
#include <firewall.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>
#include <nvmem.h>

#include <stm32_bsec3.h>
#include <tfm_plat_otp.h>

/* BSEC REGISTER OFFSET (base relative) */
#define _BSEC_FVR(i)			(U(0x000) + 4U * (i))
#define _BSEC_SPLOCK(i)			(U(0x800) + 4U * (i))
#define _BSEC_SWLOCK(i)			(U(0x840) + 4U * (i))
#define _BSEC_SRLOCK(i)			(U(0x880) + 4U * (i))
#define _BSEC_OTPVLDR(i)		(U(0x8C0) + 4U * (i))
#define _BSEC_SFSR(i)			(U(0x940) + 4U * (i))
#define _BSEC_OTPCR			U(0xC04)
#define _BSEC_WDR			U(0xC08)
#define _BSEC_LOCKR			U(0xE10)
#define _BSEC_DENR			U(0xE20)
#define _BSEC_SR			U(0xE40)
#define _BSEC_OTPSR			U(0xE44)
#define _BSEC_VERR			U(0xFF4)
#define _BSEC_IPIDR			U(0xFF8)

/* BSEC_OTPSR register fields */
#define _BSEC_OTPSR_BUSY		BIT(0)
#define _BSEC_OTPSR_INIT_DONE		BIT(1)
#define _BSEC_OTPSR_HIDEUP		BIT(2)
#define _BSEC_OTPSR_OTPNVIR		BIT(4)
#define _BSEC_OTPSR_OTPERR		BIT(5)
#define _BSEC_OTPSR_OTPSEC		BIT(6)
#define _BSEC_OTPSR_PROGFAIL		BIT(16)
#define _BSEC_OTPSR_DISTURBF		BIT(17)
#define _BSEC_OTPSR_DEDF		BIT(18)
#define _BSEC_OTPSR_SECF		BIT(19)
#define _BSEC_OTPSR_PPLF		BIT(20)
#define _BSEC_OTPSR_PPLMF		BIT(21)
#define _BSEC_OTPSR_AMEF		BIT(22)

/* BSEC_LOCKR register fields */
#define _BSEC_LOCKR_GWLOCK_MASK		BIT(0)

/* BSEC_DENR register fields */
#define _BSEC_DENR_ALL_MSK		GENMASK(15, 0)

#define _BSEC_DENR_KEY			0xDEB60000

/* BSEC_SR register fields */
#define _BSEC_SR_HVALID_MASK		BIT(1)
#define _BSEC_SR_HVALID_SHIFT		0
#define _BSEC_SR_NVSTATES_MASK		GENMASK_32(31, 26)
#define _BSEC_SR_NVSTATES_SHIFT		26

#define _BSEC_SR_NVSTATES_OPEN		U(0x16)
#define _BSEC_SR_NVSTATES_CLOSED	U(0x0D)
#define _BSEC_SR_NVSTATES_OTP_LOCKED	U(0x23)

#define _OTP_ACCESS_SIZE			12U

#define _HIDEUP_ERROR			(LOCK_SHADOW_R | LOCK_SHADOW_W | \
					 LOCK_SHADOW_P | LOCK_ERROR)

/* 32 bit by OTP bank in each register */
#define _BSEC_OTP_BIT_MASK		GENMASK_32(4, 0)
#define _BSEC_OTP_BIT_SHIFT		0
#define _BSEC_OTP_BANK_MASK		GENMASK_32(31, 5)
#define _BSEC_OTP_BANK_SHIFT		5U

#define _MAX_NB_TRIES			3U

/* Timeout when polling on status */
#define _BSEC_TIMEOUT_US		U(10000)

/* OTP18 = BOOTROM_CONFIG_0-3: Security life-cycle word 2 */
#define _OTP_SECURE_BOOT		18U
#define _OTP_CLOSED_SECURE		GENMASK_32(3, 0)

/*
 * otp shadow depend of TDCID loader
 * which copies bsec otp to shadow memory.
 * must be aligned with [TDCID loader]stm32_bsec3 driver
 */
#ifdef STM32MP215Cxx
#define STM32MP2_OTP_MAX_ID		363
#define OTP_MAX_SIZE			(STM32MP2_OTP_MAX_ID + 1U)
#else
#define STM32MP2_OTP_MAX_ID		367
#define OTP_MAX_SIZE			(STM32MP2_OTP_MAX_ID + 1U)
#endif

#define STM32MP2_UPPER_BASE		256

struct nvmem_cell {
	const char *cell_label;
	uint32_t otp_id;
	uint32_t n_otp;
	const uint32_t *shadow_value;
	uint32_t n_shadow_value;
};

struct bsec_mirror {
	uint32_t magic;
	uint32_t state;
	struct {
		uint32_t value;
		uint32_t status;
	} otp[OTP_MAX_SIZE];
};

struct stm32_bsec_config {
	uintptr_t base;
	uintptr_t mirror_addr;
	size_t mirror_size;
	const struct firewall_spec *firewall_ctrls;
	const int n_firewall_ctrls;
	const struct nvmem_cell *otp_cell;
	int n_otp_cell;
};

struct stm32_bsec_variant {
	uint32_t max_id;
	unsigned int oem_key_first_otp;
};

struct stm32_bsec_data {
	const struct stm32_bsec_variant *variant;
	bool hw_key_valid;
	struct bsec_mirror *p_mirror;
};

static const struct device *bsec_dev;

static int bsec_get_semaphore(void)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	struct firewall_spec *firewall;
	int i, ret;

	for_each_firewall(drv_cfg->firewall_ctrls, firewall, drv_cfg->n_firewall_ctrls, i){
		ret = firewall_acquire_access(firewall);
		if(ret && ret != -ENODEV){
			EMSG("Error aquire sem\n");
			return ret;
		}
	}

	return 0;
}

static int bsec_release_semaphore(void)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	struct firewall_spec *firewall;
	int i, ret;

	for_each_firewall(drv_cfg->firewall_ctrls, firewall, drv_cfg->n_firewall_ctrls, i){
		ret = firewall_release_access(firewall);
		if(ret && ret != -ENODEV){
			EMSG("Error release sem\n");
			return ret;
		}
	}
	return 0;
}

static bool is_bsec_write_locked(void)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);

	return (mmio_read_32(drv_cfg->base + _BSEC_LOCKR) &
		_BSEC_LOCKR_GWLOCK_MASK) != 0U;
}

static int shadow_otp(const struct device *dev, uint32_t otp)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);
	struct bsec_mirror *mirror = drv_data->p_mirror;
	uint32_t i, err, sr = 0;

	if (mirror) {
		/* if shadow is not allowed */
		if (mirror->otp[otp].status & LOCK_SHADOW_R) {
			mirror->otp[otp].status |= LOCK_ERROR;
			mirror->otp[otp].value = 0x0U;
			return -EACCES;
		}

		mirror->otp[otp].status &= ~LOCK_ERROR;
	}

	for (i = 0U; i < _MAX_NB_TRIES; i++) {
		io_write32(drv_cfg->base + _BSEC_OTPCR, otp);

		err = mmio_read32_poll_timeout(drv_cfg->base + _BSEC_OTPSR, sr,
					       (!(sr & _BSEC_OTPSR_BUSY)),
					       _BSEC_TIMEOUT_US);

		if (err) {
			EMSG("BSEC busy timeout\n");
			panic();
		}

		/* Retry on error */
		if (sr & (_BSEC_OTPSR_AMEF | _BSEC_OTPSR_DISTURBF |
			  _BSEC_OTPSR_DEDF))
			continue;

		/* break for OTP correctly shadowed */
		break;
	}

	if (mirror && sr & _BSEC_OTPSR_PPLF)
		mirror->otp[otp].status |= LOCK_PERM;

	if (i == _MAX_NB_TRIES || sr & (_BSEC_OTPSR_PPLMF | _BSEC_OTPSR_AMEF |
					_BSEC_OTPSR_DISTURBF |
					_BSEC_OTPSR_DEDF)) {
		if (mirror)
			mirror->otp[otp].status |= LOCK_ERROR;

		return -EIO;
	}

	return 0;
}

static bool is_fuse_shadowed(uint32_t otp)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	uint32_t bank = _FLD_GET(_BSEC_OTP_BANK, otp);
	uint32_t mask = BIT(_FLD_GET(_BSEC_OTP_BIT, otp));
	uint32_t bank_value = io_read32(drv_cfg->base + _BSEC_SFSR(bank));

	if (bank_value & mask)
		return true;

	return false;
}

/*
 * bsec_read_otp: read an OTP data value.
 * val: read value.
 * otp: OTP number.
 * return value: 0 if no error.
 */
static int __maybe_unused stm32_bsec_read_otp(uint32_t *val, uint32_t otp)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	int ret, sem_ret;

	if (!val || otp > drv_data->variant->max_id)
		return -EINVAL;

	sem_ret = bsec_get_semaphore();
	if (sem_ret)
		return sem_ret;
	*val = 0U;
	ret = shadow_otp(bsec_dev, otp);
	if (!ret)
		*val = io_read32(drv_cfg->base + _BSEC_FVR(otp));

	sem_ret = bsec_release_semaphore();
	if (sem_ret)
		return sem_ret;

	return ret;
}

/*
 * bsec_shadow_read_otp: Load OTP from SAFMEM and provide its value
 * val: read value.
 * otp: OTP number.
 * return value: 0 if no error.
 */
static int __maybe_unused stm32_bsec_shadow_read_otp(uint32_t *val,
						     uint32_t otp)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	int sem_ret, ret = 0;

	if (!val || otp > drv_data->variant->max_id)
		return -EINVAL;

	sem_ret = bsec_get_semaphore();
	if (sem_ret)
		return sem_ret;
	*val = 0U;
	if (!is_fuse_shadowed(otp))
		ret = shadow_otp(bsec_dev, otp);
	if (!ret)
		*val = io_read32(drv_cfg->base + _BSEC_FVR(otp));

	sem_ret = bsec_release_semaphore();
	if (sem_ret)
		return sem_ret;

	return ret;
}

/*
 * bsec_write_otp: write value in BSEC data register.
 * val: value to write.
 * otp: OTP number.
 * return value: 0 if no error.
 */
static int __maybe_unused stm32_bsec_write_otp(uint32_t val, uint32_t otp)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	bool value = false;
	int ret, sem_ret;

	if (otp > drv_data->variant->max_id)
		return -EINVAL;

	sem_ret = bsec_get_semaphore();
	if (sem_ret)
		return sem_ret;

	if (is_bsec_write_locked()){
		ret = -EPERM;
		goto err;
	}

	/* for HW shadowed OTP, update value in FVR register */
	if (is_fuse_shadowed(otp)) {
		ret = stm32_bsec_read_sw_lock(otp, &value);
		if (ret)
			goto err;

		if (value){
			ret = -EPERM;
			goto err;
		}

		io_write32(drv_cfg->base + _BSEC_FVR(otp), val);
	}

	ret = 0;

err:
	sem_ret = bsec_release_semaphore();
	if (sem_ret)
		return sem_ret;

	return ret;
}

void stm32_bsec_write_debug_conf(uint32_t val)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	uint32_t masked_val = val & _BSEC_DENR_ALL_MSK;
	int sem_ret;

	sem_ret = bsec_get_semaphore();
	if (sem_ret)
		return;
	if (is_bsec_write_locked())
		panic();

	mmio_write_32(drv_cfg->base + _BSEC_DENR,
		      _BSEC_DENR_KEY | masked_val);

	bsec_release_semaphore();
}

static inline int _otp_is_valid(uint32_t status)
{
	return !(status & (STATUS_SECURE | LOCK_ERROR));
}

static int __maybe_unused stm32_is_otp_dummy_provisionable(uint32_t otp_id)
{
	uint32_t iak_start, entropy_seed_start;
	uint32_t __maybe_unused bl2_rotpk_0_start;
	uint32_t ret;

	ret = stm32_bsec_get_otp_cell_by_label("entropy_seed",
					       &entropy_seed_start, NULL);
	if (ret)
		return ret;

	ret = stm32_bsec_get_otp_cell_by_label("iak", &iak_start, NULL);
	if (ret)
		return ret;

	if (otp_id == entropy_seed_start || otp_id == iak_start)
		return true;

#if defined(STM32_BL2)
	ret = stm32_bsec_get_otp_cell_by_label("bl2_rotpk_0", &bl2_rotpk_0_start, NULL);
	if (ret)
		return ret;

	if (otp_id == bl2_rotpk_0_start)
		return true;
#endif

	return false;
}

int stm32_bsec_get_otp_cell_by_label(char* label, uint32_t *cell_start,
				     uint32_t *cell_size)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	struct nvmem_cell cell;

	for (int i = 0; i < drv_cfg->n_otp_cell; i++) {
		cell = drv_cfg->otp_cell[i];

		if (!strcmp(cell.cell_label, label)) {
			if (cell_start)
				*cell_start = cell.otp_id;
			if (cell_size)
				*cell_size = cell.n_otp;
			return 0;
		}
	}
	return -ENOENT;
}

static int _otp_read(uint32_t otp_id, size_t len, size_t out_len, uint8_t *out)
{
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	struct bsec_mirror *mirror = drv_data->p_mirror;
	size_t copy_size = len < out_len ? len : out_len;
	uint32_t *p_out_w = (uint32_t *)out;
	uint32_t idx;

	if (copy_size % (sizeof(uint32_t)))
		return -EINVAL;

	for (idx = 0; idx < (copy_size / sizeof(uint32_t)); idx++) {
		if (mirror) {
			if (!_otp_is_valid(mirror->otp[otp_id + idx].status))
				return -EPERM;

			p_out_w[idx] = mirror->otp[otp_id + idx].value;
		} else {
			uint32_t val;
			int res;

			res = stm32_bsec_shadow_read_otp(&val, otp_id + idx);
			if (res) {
				memset(p_out_w, 0, copy_size);
				return -EIO;
			}

			p_out_w[idx] = val;
		}
	}

	return 0;
}

#define BOOTROM_CFG_9_SEC_BOOT	GENMASK(3,0)
#define BOOTROM_CFG_9_PROV_DONE	GENMASK(7,4)
#define HCONF1_DISABLE_SCAN	BIT(20)
static int _read_lcs(uint32_t out_len, uint8_t *out)
{
	uint32_t secure_boot, disable_scan, prov_done;
	enum plat_otp_lcs_t *lcs = (enum plat_otp_lcs_t*) out;
	uint32_t bootrom_cfg_9, hconf1;
	uint32_t cell_start, cell_size;
	int res;

	*lcs = PLAT_OTP_LCS_ASSEMBLY_AND_TEST;

	res = stm32_bsec_get_otp_cell_by_label("bootrom_config_9", &cell_start,
					       &cell_size);
	if (res)
		return res;

	res = _otp_read(cell_start, cell_size * sizeof(uint32_t),
			sizeof(bootrom_cfg_9), (uint8_t *)&bootrom_cfg_9);
	if (res)
		return res;

	res = stm32_bsec_get_otp_cell_by_label("hconf1_otp", &cell_start,
					       &cell_size);
	if (res)
		return res;

	res = _otp_read(cell_start, cell_size * sizeof(uint32_t),
			sizeof(hconf1), (uint8_t *)&hconf1);
	if (res)
		return res;

	/* true if all bit of field are set */
	secure_boot = !!(bootrom_cfg_9 & BOOTROM_CFG_9_SEC_BOOT);
	prov_done = !!(bootrom_cfg_9 & BOOTROM_CFG_9_PROV_DONE);
	disable_scan = !!(hconf1 & HCONF1_DISABLE_SCAN);

	if (secure_boot && prov_done && disable_scan)
		*lcs = PLAT_OTP_LCS_SECURED;

	return 0;
}

static int __maybe_unused _otp_write(uint32_t otp_id, size_t len,
				     size_t in_len, const uint8_t *in)
{
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	struct bsec_mirror *mirror = drv_data->p_mirror;
	uint32_t *p_in_w = (uint32_t *)in;
	uint32_t idx;

	if (len != in_len)
		return -EINVAL;

	if (len % (sizeof(uint32_t)))
		return -EINVAL;

	for (idx = 0; idx < len / sizeof(uint32_t); idx++) {
		if (mirror) {
			if (!_otp_is_valid(mirror->otp[otp_id + idx].status))
				return -EPERM;

			mirror->otp[otp_id + idx].value = p_in_w[idx];
			mirror->otp[otp_id + idx].status = LOCK_SHADOW_R;
		} else {
			if (stm32_bsec_write_otp(p_in_w[idx], otp_id + idx))
				return -EIO;
		}
	}

	return 0;
}

static int __maybe_unused _otp_write_lcs(uint32_t in_len, const uint8_t *in)
{
	enum plat_otp_lcs_t *lcs = (enum plat_otp_lcs_t*) in;
	uint32_t bootrom_cfg_9, cfg9_start, cfg9_size;
	uint32_t hconf1, hconf1_start, hconf1_size;
	int res;

	res = stm32_bsec_get_otp_cell_by_label("bootrom_config_9", &cfg9_start,
					       &cfg9_size);
	if (res)
		return res;

	res = _otp_read(cfg9_start, cfg9_size * sizeof(uint32_t),
			sizeof(bootrom_cfg_9), (uint8_t *)&bootrom_cfg_9);
	if (res)
		return res;

	res = stm32_bsec_get_otp_cell_by_label("hconf1_otp", &hconf1_start,
					       &hconf1_size);
	if (res)
		return res;

	res = _otp_read(hconf1_start, hconf1_size * sizeof(uint32_t),
			sizeof(hconf1), (uint8_t *)&hconf1);
	if (res)
		return res;

	if (*lcs == PLAT_OTP_LCS_SECURED) {
		if (!(bootrom_cfg_9 & BOOTROM_CFG_9_SEC_BOOT))
			bootrom_cfg_9 |= BOOTROM_CFG_9_SEC_BOOT;
		if (!(bootrom_cfg_9 & BOOTROM_CFG_9_PROV_DONE))
			bootrom_cfg_9 |= BOOTROM_CFG_9_PROV_DONE;
		if (!(hconf1 & HCONF1_DISABLE_SCAN))
			hconf1 |= HCONF1_DISABLE_SCAN;
	} else {
		bootrom_cfg_9 &= ~BOOTROM_CFG_9_PROV_DONE;
	}

	res = _otp_write(cfg9_start, cfg9_size * sizeof(uint32_t),
			 sizeof(bootrom_cfg_9), (uint8_t*)&bootrom_cfg_9);
	if (res)
		return res;

	res = _otp_write(hconf1_start, hconf1_size * sizeof(uint32_t),
			 sizeof(hconf1), (uint8_t*)&hconf1);
	if (res)
		return res;

	return 0;
}

/*
 * STM32 driver Interface
 */
int stm32_bsec_read_sw_lock(uint32_t otp, bool *value)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	const struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	int sem_ret;

	if (!value)
		return -EINVAL;

	if (otp > drv_data->variant->max_id)
		return -EINVAL;

	if (drv_data->p_mirror) {
		*value = !!(drv_data->p_mirror->otp[otp].status & LOCK_SHADOW_W);
	} else {
		uint32_t bank = _FLD_GET(_BSEC_OTP_BANK, otp);
		uint32_t mask = BIT(_FLD_GET(_BSEC_OTP_BIT, otp));

		sem_ret = bsec_get_semaphore();
		if (sem_ret)
			return sem_ret;

		*value = !!(io_read32(drv_cfg->base + _BSEC_SWLOCK(bank)) & mask);

		sem_ret = bsec_release_semaphore();
		if (sem_ret)
			return sem_ret;
	}

	return 0;
}

int stm32_bsec_write(uint32_t otp, uint32_t value)
{
	const struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);

	if (stm32_bsec_write_otp(value, otp))
		return -EIO;

	/* update bsec mirror */
	if (drv_data->p_mirror)
		drv_data->p_mirror->otp[otp].value = value;

	return 0;
}

/*
 * Interface with TFM
 */
int stm32_bsec_otp_read_by_id(enum tfm_otp_element_id_t id, size_t out_len,
			      uint8_t *out)
{
	int res;
	uint32_t cell_start;
	uint32_t cell_size;

	switch (id) {
	case PLAT_OTP_ID_LCS:
		return _read_lcs(out_len, out);
	case PLAT_OTP_ID_IAK_LEN:
		res = stm32_bsec_get_otp_cell_by_label("iak", NULL, &cell_size);
		*out = cell_size * sizeof(uint32_t);

		return res;

	case PLAT_OTP_ID_IAK:
		res = stm32_bsec_get_otp_cell_by_label("iak", &cell_start,
						       &cell_size);
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		res = stm32_bsec_get_otp_cell_by_label("implementation_id",
						       &cell_start, &cell_size);
		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		res = stm32_bsec_get_otp_cell_by_label("entropy_seed",
						       &cell_start, &cell_size);
		break;
#if defined(STM32_BL2)
	/*
	 * For now, we use the same key for each software image loaded by
	 * MCUBoot.
	 */
	case PLAT_OTP_ID_BL2_ROTPK_0:
	case PLAT_OTP_ID_BL2_ROTPK_1:
	case PLAT_OTP_ID_BL2_ROTPK_2:
	case PLAT_OTP_ID_BL2_ROTPK_3:
		/* Image id 1 (supposed to use rotpk1) is DDR Firmware.
		 * We choose to use the Secure world key to sign it
		 * (rotpk0).
		 */
		res = stm32_bsec_get_otp_cell_by_label("bl2_rotpk_0",
						       &cell_start, &cell_size);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (res)
		return res;

	return _otp_read(cell_start, cell_size * sizeof(uint32_t), out_len,
			 out);

}

int stm32_bsec_otp_size_by_id(enum tfm_otp_element_id_t id, size_t *size)
{
	int res = 0;
	uint32_t cell_size;

	switch (id) {
	case PLAT_OTP_ID_LCS:
		*size = sizeof(uint32_t);
		break;
	case PLAT_OTP_ID_IAK:
		res = stm32_bsec_get_otp_cell_by_label("iak", NULL, &cell_size);

		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		res = stm32_bsec_get_otp_cell_by_label("implementation_id",
						       NULL, &cell_size);

		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		res = stm32_bsec_get_otp_cell_by_label("entropy_seed", NULL,
						       &cell_size);

		break;
#if defined(STM32_BL2)
	case PLAT_OTP_ID_BL2_ROTPK_0:
	case PLAT_OTP_ID_BL2_ROTPK_2:
	case PLAT_OTP_ID_BL2_ROTPK_3:
		res = stm32_bsec_get_otp_cell_by_label("bl2_rotpk_0", NULL,
						       &cell_size);

		break;
	case PLAT_OTP_ID_BL2_ROTPK_1:
		res = stm32_bsec_get_otp_cell_by_label("bl2_rotpk_1", NULL,
						       &cell_size);

		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (res)
		return res;

	*size = cell_size * sizeof(uint32_t);

	return 0;
}

static int stm32_bsec_nvmem_get_cell_size(const struct device *dev, size_t *size)
{
	const struct nvmem_cell *cell = dev_get_config(dev);

	if (!cell)
		return -EINVAL;

	*size = cell->n_otp * sizeof(uint32_t);

	return 0;
}

static int stm32_bsec_nvmem_read_cell(const struct device *dev, size_t out_len, uint8_t *out,
			       size_t *read_len)
{
	const struct nvmem_cell *cell = dev_get_config(dev);
	int res;

	if (!cell ||  out_len > cell->n_otp * sizeof(uint32_t))
		return -EINVAL;

	res = _otp_read(cell->otp_id, cell->n_otp * sizeof(uint32_t), out_len, out);
	if (res) {
		memset(out, 0, out_len);
		return res;
	}

	*read_len = out_len;

	return 0;
}

static int stm32_bsec_nvmem_write_cell(const struct device *dev, size_t in_len, const uint8_t *in)
{
	return -ENOTSUP;
}

bool stm32_bsec_is_valid(void)
{
	return device_is_ready(bsec_dev);
}

static void stm32_bsec_check_error(uint32_t opt_status)
{
	if (opt_status & _BSEC_OTPSR_OTPSEC)
		DMSG("BSEC reset single error correction detected\n");

	if (!(opt_status & _BSEC_OTPSR_OTPNVIR))
		DMSG("BSEC virgin OTP word 0\n");

	if (opt_status & _BSEC_OTPSR_HIDEUP)
		DMSG("BSEC upper fuse not accessible\n");

	if (opt_status & _BSEC_OTPSR_OTPERR) {
		EMSG("BSEC shadow error detected\n");
		panic();
	}

	if (!(opt_status & _BSEC_OTPSR_INIT_DONE)) {
		EMSG("BSEC reset operations not completed\n");
		panic();
	}

	if (is_bsec_write_locked()) {
		EMSG("BSEC global write lock\n");
		panic();
	}
}

static uint32_t init_state(const struct device *dev, uint32_t status)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);
	struct bsec_mirror *mirror = drv_data->p_mirror;
	uint32_t state = BSEC_STATE_INVALID;

	if (status & _BSEC_OTPSR_INIT_DONE) {
		/* NVSTATES is only valid if INIT_DONE = 1 */
		uint32_t sr = io_read32(drv_cfg->base + _BSEC_SR);
		uint32_t nvstates = _FLD_GET(_BSEC_SR_NVSTATES, sr);

		/* Only 1 supported state = CLOSED */
		if (nvstates != _BSEC_SR_NVSTATES_CLOSED) {
			state = BSEC_STATE_INVALID;
			EMSG("BSEC invalid nvstates %#x\n", nvstates);
		} else {
			state = BSEC_STATE_SEC_OPEN;
			if (mirror->otp[_OTP_SECURE_BOOT].value & _OTP_CLOSED_SECURE)
				state = BSEC_STATE_SEC_CLOSED;
		}
	}

	return state;
}


static void stm32_bsec_mirror_load(const struct device *dev, uint32_t status)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);
	unsigned int otp = 0U, bank = 0U;
	//uint32_t exceptions = 0U;
	uint32_t srlock[_OTP_ACCESS_SIZE] = { 0U };
	uint32_t swlock[_OTP_ACCESS_SIZE] = { 0U };
	uint32_t splock[_OTP_ACCESS_SIZE] = { 0U };
	uint32_t mask = 0U;
	unsigned int max_id = drv_data->variant->max_id;

	memset(drv_data->p_mirror, 0, sizeof(*drv_data->p_mirror));
	drv_data->p_mirror->magic = BSEC_MAGIC;
	drv_data->p_mirror->state = BSEC_STATE_INVALID;

	/* HIDEUP: read and write not possible in upper region */
	if (status & _BSEC_OTPSR_HIDEUP) {
		for (otp = STM32MP2_UPPER_BASE;
		     otp <= drv_data->variant->max_id ; otp++) {
			drv_data->p_mirror->otp[otp].status |= _HIDEUP_ERROR;
			drv_data->p_mirror->otp[otp].value = 0x0U;
		}
		max_id = STM32MP2_UPPER_BASE - 1;
	}

	for (bank = 0U; bank < _OTP_ACCESS_SIZE; bank++) {
		srlock[bank] = io_read32(drv_cfg->base + _BSEC_SRLOCK(bank));
		swlock[bank] = io_read32(drv_cfg->base + _BSEC_SWLOCK(bank));
		splock[bank] = io_read32(drv_cfg->base + _BSEC_SPLOCK(bank));
	}

	for (otp = 0U; otp <= max_id ; otp++) {
		int ret;

		bank = _FLD_GET(_BSEC_OTP_BANK, otp);
		mask = BIT(_FLD_GET(_BSEC_OTP_BIT, otp));

		if (srlock[bank] & mask)
			drv_data->p_mirror->otp[otp].status |= LOCK_SHADOW_R;
		if (swlock[bank] & mask)
			drv_data->p_mirror->otp[otp].status |= LOCK_SHADOW_W;
		if (splock[bank] & mask)
			drv_data->p_mirror->otp[otp].status |= LOCK_SHADOW_P;

		if (drv_data->p_mirror->otp[otp].status & STATUS_SECURE)
			continue;

		/*
		 * OEM keys are accessible only in ROM code
		 * They are stored in last OTPs
		 */
		if (otp >= drv_data->variant->oem_key_first_otp) {
			drv_data->p_mirror->otp[otp].status |= LOCK_SHADOW_R;
			continue;
		}

		if (!(drv_data->p_mirror->otp[otp].status & LOCK_SHADOW_R)) {
			/* reload shadow to read Permanent Programing Lock Flag */

			ret = shadow_otp(dev, otp);
			if (ret) {
				EMSG("Shadowing failed (%d)\n", ret);
				return;
			}
		}

		drv_data->p_mirror->otp[otp].value = io_read32(drv_cfg->base +
							   _BSEC_FVR(otp));
	}

}

static void stm32_bsec_mirror_init(const struct device *dev, bool force_load)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);
	struct bsec_mirror *mirror = drv_data->p_mirror;
	uint32_t status;

	if (bsec_get_semaphore())
		return;

	status = io_read32(drv_cfg->base + _BSEC_OTPSR);
	stm32_bsec_check_error(status);

	/* update mirror when forced or invalid */
	if (force_load || mirror->magic != BSEC_MAGIC)
		stm32_bsec_mirror_load(dev, status);

	/* always update status */
	mirror->state = init_state(dev, status);
	if ((mirror->state & BSEC_STATE_MASK) == BSEC_STATE_INVALID) {
		EMSG("BSEC invalid state\n");
		panic();
	}

	bsec_release_semaphore();
}

static int stm32_bsec_shadow_init(const struct device *dev)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	const struct nvmem_cell *cell = NULL;
	bool sw_lock;
	int i, j, ret;

	for (i = 0; i < drv_cfg->n_otp_cell; i++) {
		cell = &drv_cfg->otp_cell[i];

		if (cell->n_shadow_value == 0)
			continue;

		/*
		 * The shadow_value array must have a value for all
		 * the OTP of the section.
		 */
		if (cell->n_shadow_value != cell->n_otp) {
			EMSG("size of shadow-provisionning not equal to size of"
			     " reg for node otp : %d\n",
			     cell->otp_id);
			return -EINVAL;
		}

		for (j = 0; j < cell->n_otp; j++) {

			/* no shadow value to provision, skip OTP */
			if (cell->shadow_value[j] == 0)
				continue;

			/* ensure shadow write is allowed */
			ret = stm32_bsec_read_sw_lock(cell->otp_id + j,
						      &sw_lock);
			if (ret)
				return ret;

			if (sw_lock)
				return -EACCES;

			stm32_bsec_write(cell->otp_id + j,
					 cell->shadow_value[j]);

			/* update bsec mirror */
			ret = _otp_write(cell->otp_id, cell->n_otp * sizeof(uint32_t),
					 cell->n_shadow_value * sizeof(uint32_t),
					 (uint8_t *)cell->shadow_value);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int stm32_bsec_dt_init(const struct device *dev)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);

	drv_data->hw_key_valid = false;

	if (IS_ENABLED(STM32_BL2)) {
		drv_data->hw_key_valid = !!(io_read32(drv_cfg->base + _BSEC_SR) &
					    _BSEC_SR_HVALID_MASK);
		drv_data->p_mirror = NULL;
	} else {
		drv_data->p_mirror = (struct bsec_mirror *)drv_cfg->mirror_addr;

		if (IS_ENABLED(STM32_M33TDCID))
			stm32_bsec_mirror_init(dev, true);

		if (drv_data->p_mirror->magic != BSEC_MAGIC)
			return -ENOSYS;

		if (drv_data->p_mirror->state & BSEC_HARDWARE_KEY)
			drv_data->hw_key_valid = true;
	}

	return stm32_bsec_shadow_init(dev);
}

static const struct nvmem_driver_api __maybe_unused stm32_bsec_nvmem_api = {
	.get_cell_size = stm32_bsec_nvmem_get_cell_size,
	.read_cell = stm32_bsec_nvmem_read_cell,
	.write_cell = stm32_bsec_nvmem_write_cell,
};

#define NVMEM_CELL_CHILD_DEFINE(node_id)					\
static const uint32_t shadow_value_##node_id[] =				\
	DT_PROP_OR(node_id, shadow_provisionning, {});				\
										\
static const char * const stm32_otp_label_##node_id[] =				\
	DT_NODELABEL_STRING_ARRAY(node_id);					\
										\
static const struct nvmem_cell stm32_otp_cell_##node_id = {			\
	.cell_label = stm32_otp_label_##node_id[0],				\
	.otp_id = (DT_REG_ADDR(node_id) / 4),					\
	.n_otp = (DT_REG_SIZE(node_id) / 4),					\
	.shadow_value = shadow_value_##node_id,					\
	.n_shadow_value = DT_PROP_LEN_OR(node_id, shadow_provisionning, 0)	\
};										\
										\
DEVICE_DT_DEFINE(node_id, NULL,							\
		 NULL,								\
		 &stm32_otp_cell_##node_id,					\
		 CORE, 6,							\
		 &stm32_bsec_nvmem_api);


#define NVMEM_CELL_CHILD_GET(node_id) stm32_otp_cell_##node_id,

#define STM32_BSEC3_INIT(node_id, _variant)					\
										\
DT_FOREACH_CHILD(node_id, NVMEM_CELL_CHILD_DEFINE)				\
										\
static const struct nvmem_cell stm32_otp_cells_##node_id [] = {			\
	DT_FOREACH_CHILD(node_id, NVMEM_CELL_CHILD_GET)				\
};										\
										\
DT_ACCESS_CTRLS_DEFINE(node_id);						\
										\
static const struct stm32_bsec_config stm32_bsec3_cfg_ ## node_id = {		\
	.base = DT_REG_ADDR(node_id),						\
	.mirror_addr = DT_REG_ADDR(DT_PHANDLE(node_id, memory_region)),		\
	.mirror_size = DT_REG_SIZE(DT_PHANDLE(node_id, memory_region)),		\
	.firewall_ctrls = DT_ACCESS_CTRLS_GET(node_id),				\
	.n_firewall_ctrls = DT_ACCESS_CTRLS_NUM(node_id),			\
	.otp_cell = stm32_otp_cells_##node_id,					\
	.n_otp_cell = ARRAY_SIZE(stm32_otp_cells_##node_id),			\
};										\
										\
static struct stm32_bsec_data stm32_bsec3_data_ ## node_id = {			\
	.variant = _variant,							\
};										\
										\
static const struct device *bsec_dev = DEVICE_DT_INST_GET(0);			\
										\
DEVICE_DT_DEFINE(node_id, &stm32_bsec_dt_init,					\
		 &stm32_bsec3_data_##node_id,					\
		 &stm32_bsec3_cfg_##node_id,					\
		 CORE, 5,							\
		 NULL);



static __unused struct stm32_bsec_variant variant_stm32mp21 = {
/*
 * BSEC: 364 available OTPs, the other are masked
 * - OEM FSBL keys 348 to 363 (programmable but not readable)
 * - ECIES key: 364 to 375 (only readable by bootrom)
 * - HWKEY: 376 to 383 (never reloadable or readable)
 */
	.oem_key_first_otp = 348,
	.max_id = STM32MP2_OTP_MAX_ID,
};

static __unused struct stm32_bsec_variant variant_stm32mp25 = {
/*
 * BSEC: 368 available OTPs, the other are masked
 * - OEM FSBL keys 360 to 367 (programmable but not readable)
 * - ECIES key: 368 to 375 (only readable by bootrom)
 * - HWKEY: 376 to 383 (never reloadable or readable)
 */
	.oem_key_first_otp = 360,
	.max_id = STM32MP2_OTP_MAX_ID,
};

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32mp21_bsec

DT_FOREACH_STATUS_OKAY_VARGS(st_stm32mp21_bsec, STM32_BSEC3_INIT,
			     &variant_stm32mp21)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT		st_stm32mp25_bsec

DT_FOREACH_STATUS_OKAY_VARGS(st_stm32mp25_bsec, STM32_BSEC3_INIT,
			     &variant_stm32mp25)
