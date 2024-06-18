/*
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 * Author(s): Ludovic Barre, <ludovic.barre@st.com> for STMicroelectronics.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#define DT_DRV_COMPAT st_stm32mp25_bsec

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmsis_compiler.h>

#include <device.h>
#include <debug.h>
#include <lib/mmio.h>
#include <lib/mmiopoll.h>
#include <lib/utils_def.h>

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
#define _BSEC_OTPSR_FUSEOK		BIT(1)
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

/* OEM Keys are stored from _OEM_KEY_FIRST_OTP to OTP bsec_dev.max_id (367) */
#define _OEM_KEY_FIRST_OTP		360

struct nvmem_cell {
	uint32_t otp_id;
	uint32_t n_otp;
	const uint32_t *shadow_value;
	uint32_t n_shadow_value;
};

struct bsec_shadow {
	uint32_t magic;
	uint32_t state;
	uint32_t value[OTP_MAX_SIZE];
	uint32_t status[OTP_MAX_SIZE];
};

struct stm32_bsec_config {
	uintptr_t base;
	uintptr_t mirror_addr;
	size_t mirror_size;
	const struct nvmem_cell *otp_cell;
	int n_otp_cell;
};

struct stm32_bsec_variant {
	uint32_t upper_base;
	uint32_t max_id;
};

struct stm32_bsec_data {
	const struct stm32_bsec_variant *variant;
	bool hw_key_valid;
	struct bsec_shadow *p_shadow;
#ifdef TFM_DUMMY_PROVISIONING
	__PACKED_STRUCT bsec_shadow shadow_dummy;
#endif
};

static const struct device *bsec_dev = DEVICE_DT_INST_GET(0);

static void bsec_lock(void)
{
	/* Not yet available */
	return;
}

static void bsec_unlock(void)
{
	/* Not yet available */
	return;
}

static bool is_bsec_write_locked(void)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);

	return (mmio_read_32(drv_cfg->base + _BSEC_LOCKR) &
		_BSEC_LOCKR_GWLOCK_MASK) != 0U;
}

void stm32_bsec_write_debug_conf(uint32_t val)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);

	uint32_t masked_val = val & _BSEC_DENR_ALL_MSK;

	if (is_bsec_write_locked() == true) {
		panic();
	}

	bsec_lock();
	mmio_write_32(drv_cfg->base + _BSEC_DENR,
		      _BSEC_DENR_KEY | masked_val);
	bsec_unlock();
}

/*
 * TDCID populate a memory area with bsec otp.
 * A filter is apply to not miror all otp.
 * This area is read only and share whith cortex A and M.
 */
__PACKED_STRUCT otp_shadow_layout_t {
	uint32_t reserved1[5];
	uint32_t implementation_id[3];		/* otp 5..7 */
	uint32_t reserved2[10];
	uint32_t bootrom_config_9[1];		/* otp 18  */
	uint32_t reserved3[105];
	uint32_t hconf1[1];			/* otp 124 */
	uint32_t reserved4[51];
	uint32_t bl2_rotpk_0[8];		/* otp 176 */
	uint32_t bl2_rotpk_1[8];		/* otp 184 */
	uint32_t reserved5[140];
	uint32_t entropy_seed[16];		/* otp 332 */
	uint32_t iak[8];			/* otp 348 */
};

#define OTP_OFFSET(x)		(offsetof(struct otp_shadow_layout_t, x))
#define OTP_SIZE(x)		(sizeof(((struct otp_shadow_layout_t *)0)->x))

#define BOOTROM_CFG_9_SEC_BOOT	GENMASK(3,0)
#define BOOTROM_CFG_9_PROV_DONE	GENMASK(7,4)
#define HCONF1_DISABLE_SCAN	BIT(20)

static inline int _otp_is_valid(uint32_t status){
	return !(status & (STATUS_SECURE | LOCK_ERROR));
}

static int _otp_read(uint32_t offset, size_t len, size_t out_len, uint8_t *out)
{
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	struct bsec_shadow *shadow = drv_data->p_shadow;
	size_t copy_size = len < out_len ? len : out_len;
	uint32_t *p_out_w = (uint32_t *)out;
	uint32_t *p_value, *p_status;
	uint32_t idx;

	if (offset % (sizeof(uint32_t)))
		return -EINVAL;

	if (copy_size % (sizeof(uint32_t)))
		return -EINVAL;

	offset /= sizeof(uint32_t);
	p_value = &(shadow->value[offset]);
	p_status = &(shadow->status[offset]);

	for (idx = 0; idx < (copy_size / sizeof(uint32_t)); idx++) {
		if (!_otp_is_valid(p_status[idx]))
			return -EPERM;

		p_out_w[idx] = p_value[idx];
	}

	return 0;
}

static int _otp_read_lcs(uint32_t out_len, uint8_t *out)
{
	uint32_t secure_boot, disable_scan, prov_done;
	enum plat_otp_lcs_t *lcs = (enum plat_otp_lcs_t*) out;
	uint32_t btrom_cfg_9, hconf1;
	int err;

	*lcs = PLAT_OTP_LCS_ASSEMBLY_AND_TEST;

	err = _otp_read(OTP_OFFSET(bootrom_config_9),
			OTP_SIZE(bootrom_config_9),
			sizeof(btrom_cfg_9), (uint8_t*)&btrom_cfg_9);
	if (err)
		return err;

	err = _otp_read(OTP_OFFSET(hconf1), OTP_SIZE(hconf1),
			sizeof(hconf1), (uint8_t*)&hconf1);
	if (err)
		return err;

	/* true if any bit of field is set */
	secure_boot = !!(btrom_cfg_9 & BOOTROM_CFG_9_SEC_BOOT);
	prov_done = !!(btrom_cfg_9 & BOOTROM_CFG_9_PROV_DONE);
	disable_scan = !!(hconf1 & HCONF1_DISABLE_SCAN);

	if (secure_boot && prov_done && disable_scan)
		*lcs = PLAT_OTP_LCS_SECURED;

	return 0;
}

static int __maybe_unused _otp_write(uint32_t offset, size_t len,
				     size_t in_len, const uint8_t *in)
{
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	struct bsec_shadow *shadow = drv_data->p_shadow;
	uint32_t *p_in_w = (uint32_t *)in;
	uint32_t *p_value, *p_status;
	uint32_t idx;

	if (offset % (sizeof(uint32_t)))
		return -EINVAL;

	if (len != in_len)
		return -EINVAL;

	if (len % (sizeof(uint32_t)))
		return -EINVAL;

	offset /= sizeof(uint32_t);
	p_value = &(shadow->value[offset]);
	p_status = &(shadow->status[offset]);

	for (idx = 0; idx < (len / sizeof(uint32_t)); idx++) {
		if (!_otp_is_valid(p_status[idx]))
			return -EPERM;

		p_value[idx] = p_in_w[idx];
		p_status[idx] = LOCK_SHADOW_R;
	}

	return 0;
}

static int __maybe_unused _otp_write_lcs(uint32_t in_len, const uint8_t *in)
{
	enum plat_otp_lcs_t *lcs = (enum plat_otp_lcs_t*) in;
	uint32_t btrom_cfg_9, hconf1;
	int err;

	err = _otp_read(OTP_OFFSET(bootrom_config_9),
			OTP_SIZE(bootrom_config_9),
			sizeof(btrom_cfg_9), (uint8_t*)&btrom_cfg_9);
	if (err)
		return err;

	err = _otp_read(OTP_OFFSET(hconf1), OTP_SIZE(hconf1),
			sizeof(hconf1), (uint8_t*)&hconf1);
	if (err)
		return err;

	if (*lcs == PLAT_OTP_LCS_SECURED) {
		if (!(btrom_cfg_9 & BOOTROM_CFG_9_SEC_BOOT))
			btrom_cfg_9 |= BOOTROM_CFG_9_SEC_BOOT;

		if (!(btrom_cfg_9 & BOOTROM_CFG_9_PROV_DONE))
			btrom_cfg_9 |= BOOTROM_CFG_9_PROV_DONE;

		if (!(hconf1 & HCONF1_DISABLE_SCAN))
			hconf1 |= HCONF1_DISABLE_SCAN;
	}
	else {
		btrom_cfg_9 &= ~BOOTROM_CFG_9_PROV_DONE;
	}

	err = _otp_write(OTP_OFFSET(bootrom_config_9),
			 OTP_SIZE(bootrom_config_9),
			 sizeof(btrom_cfg_9), (uint8_t*)&btrom_cfg_9);
	if (err)
		return err;

	err = _otp_write(OTP_OFFSET(hconf1), OTP_SIZE(hconf1),
			 sizeof(hconf1), (uint8_t*)&hconf1);
	if (err)
		return err;

	return 0;
}

/*
 * STM32 driver Interface
 */
int stm32_bsec_read_sw_lock(uint32_t otp, bool *value)
{
	const struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);

	if (!value)
		return -EINVAL;

	if (otp > drv_data->variant->max_id)
		return -EINVAL;

	*value = !!(drv_data->p_shadow->status[otp] & LOCK_SHADOW_W);

	return 0;
}

int stm32_bsec_write(uint32_t otp, uint32_t value)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(bsec_dev);
	const struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);
	bool sw_lock = false;

	if (otp > drv_data->variant->max_id)
		return -EINVAL;

	if (is_bsec_write_locked())
		return -EINVAL;

	/* for HW shadowed OTP, update value in FVR register */
	stm32_bsec_read_sw_lock(otp, &sw_lock);
	if (sw_lock) {
		DMSG("BSEC: OTP %d is write locked, write ignored", otp);
		return -EACCES;
	}

	mmio_write_32(drv_cfg->base + _BSEC_FVR(otp), value);

	/* update bsec mirror */
	drv_data->p_shadow->value[otp] = value;

	return 0;
}

/*
 * Interface with TFM
 */
int stm32_bsec_otp_read_by_id(enum tfm_otp_element_id_t id, size_t out_len,
			      uint8_t *out)
{
	switch (id) {
	case PLAT_OTP_ID_LCS:
		return _otp_read_lcs(out_len, out);
	case PLAT_OTP_ID_IAK:
		return _otp_read(OTP_OFFSET(iak), OTP_SIZE(iak), out_len, out);
	case PLAT_OTP_ID_IAK_LEN:
		*out = OTP_SIZE(iak);
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		return _otp_read(OTP_OFFSET(implementation_id),
				 OTP_SIZE(implementation_id), out_len, out);
	case PLAT_OTP_ID_ENTROPY_SEED:
		return _otp_read(OTP_OFFSET(entropy_seed),
				 OTP_SIZE(entropy_seed), out_len, out);
#if defined(STM32_BL2)
	case PLAT_OTP_ID_BL2_ROTPK_0:
		return _otp_read(OTP_OFFSET(bl2_rotpk_0),
				 OTP_SIZE(bl2_rotpk_0), out_len, out);
	case PLAT_OTP_ID_BL2_ROTPK_1:
		return _otp_read(OTP_OFFSET(bl2_rotpk_1),
				 OTP_SIZE(bl2_rotpk_1), out_len, out);
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

int stm32_bsec_otp_size_by_id(enum tfm_otp_element_id_t id, size_t *size)
{
	switch (id) {
	case PLAT_OTP_ID_LCS:
		*size = sizeof(uint32_t);
		break;
	case PLAT_OTP_ID_IAK:
		*size = OTP_SIZE(iak);
		break;
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		*size = OTP_SIZE(implementation_id);
		break;
	case PLAT_OTP_ID_ENTROPY_SEED:
		*size = OTP_SIZE(entropy_seed);
		break;
#if defined(STM32_BL2)
	case PLAT_OTP_ID_BL2_ROTPK_0:
		*size = OTP_SIZE(bl2_rotpk_0);
		break;
	case PLAT_OTP_ID_BL2_ROTPK_1:
		*size = OTP_SIZE(bl2_rotpk_1);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef TFM_DUMMY_PROVISIONING
int stm32_bsec_otp_write_by_id(enum tfm_otp_element_id_t id, size_t in_len,
			       const uint8_t *in)
{
	switch (id) {
	case PLAT_OTP_ID_LCS:
		return _otp_write_lcs(in_len, in);
	case PLAT_OTP_ID_IMPLEMENTATION_ID:
		return _otp_write(OTP_OFFSET(implementation_id),
				  OTP_SIZE(implementation_id), in_len, in);
	case PLAT_OTP_ID_ENTROPY_SEED:
		return _otp_write(OTP_OFFSET(entropy_seed),
				  OTP_SIZE(entropy_seed), in_len, in);
	case PLAT_OTP_ID_IAK:
		return _otp_write(OTP_OFFSET(iak), OTP_SIZE(iak),in_len, in);
#if defined(STM32_BL2)
	case PLAT_OTP_ID_BL2_ROTPK_0:
		return _otp_write(OTP_OFFSET(bl2_rotpk_0),
				  OTP_SIZE(bl2_rotpk_0), in_len, in);
	case PLAT_OTP_ID_BL2_ROTPK_1:
		return _otp_write(OTP_OFFSET(bl2_rotpk_1),
				  OTP_SIZE(bl2_rotpk_1), in_len, in);
#endif
	default:
		return -ENOTSUP;
	}
}

int stm32_bsec_dummy_switch(void)
{
	struct stm32_bsec_data *drv_data = dev_get_data(bsec_dev);

	memcpy(&(drv_data->shadow_dummy), drv_data->p_shadow,
	       sizeof(drv_data->shadow_dummy));
	drv_data->p_shadow = &drv_data->shadow_dummy;

	return 0;
}
#else
int stm32_bsec_otp_write_by_id(enum tfm_otp_element_id_t id, size_t in_len,
			       const uint8_t *in)
{
	return -ENOTSUP;
}

int stm32_bsec_dummy_switch(void)
{
	return -ENOTSUP;
}
#endif


bool stm32_bsec_is_valid(void)
{
	return device_is_ready(bsec_dev);
}

static void stm32_bsec_check_error(uint32_t opt_status)
{
	if (opt_status & _BSEC_OTPSR_OTPSEC)
		DMSG("BSEC reset single error correction detected");

	if (!(opt_status & _BSEC_OTPSR_OTPNVIR))
		DMSG("BSEC virgin OTP word 0");

	if (opt_status & _BSEC_OTPSR_HIDEUP)
		DMSG("BSEC upper fuse not accessible");

	if (opt_status & _BSEC_OTPSR_OTPERR) {
		EMSG("BSEC shadow error detected");
		panic();
	}

	if (!(opt_status & _BSEC_OTPSR_FUSEOK)) {
		EMSG("BSEC reset operations not completed");
		panic();
	}

	if (is_bsec_write_locked()) {
		EMSG("BSEC global write lock");
		panic();
	}
}

static int shadow_otp(const struct device *dev, uint32_t otp)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);
	struct bsec_shadow *shadow = drv_data->p_shadow;
	uint32_t i, err, sr = 0;

	/* if shadow is not allowed */
	if (shadow->status[otp] & LOCK_SHADOW_R) {
		shadow->status[otp] |= LOCK_ERROR;
		shadow->value[otp] = 0x0U;
		return -EACCES;
	}

	shadow->status[otp] &= ~LOCK_ERROR;

	for (i = 0U; i < _MAX_NB_TRIES; i++) {
		io_write32(drv_cfg->base + _BSEC_OTPCR, otp);

		err = mmio_read32_poll_timeout(drv_cfg->base + _BSEC_OTPSR, sr,
					       (!(sr & _BSEC_OTPSR_BUSY)),
					       _BSEC_TIMEOUT_US);

		if (err) {
			EMSG("BSEC busy timeout");
			panic();
		}

		/* Retry on error */
		if (sr & (_BSEC_OTPSR_AMEF | _BSEC_OTPSR_DISTURBF |
			  _BSEC_OTPSR_DEDF))
			continue;

		/* break for OTP correctly shadowed */
		break;
	}

	if (sr & _BSEC_OTPSR_PPLF)
		shadow->status[otp] |= LOCK_PERM;

	if (i == _MAX_NB_TRIES || sr & (_BSEC_OTPSR_PPLMF | _BSEC_OTPSR_AMEF |
					_BSEC_OTPSR_DISTURBF |
					_BSEC_OTPSR_DEDF)) {
		shadow->status[otp] |= LOCK_ERROR;
		return -EIO;
	}

	return 0;
}

static uint32_t init_state(const struct device *dev, uint32_t status)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);
	struct bsec_shadow *shadow = drv_data->p_shadow;
	uint32_t sr, nvstates, state;

	sr = io_read32(drv_cfg->base + _BSEC_SR);
	nvstates = _FLD_GET(_BSEC_SR_NVSTATES, sr);
	state = BSEC_STATE_INVALID;

	if (status & BSEC_OTPSR_FUSEOK) {
		/*
		 * NVSTATES is only valid if FUSEOK
		 * Only 1 supported state = CLOSED
		 */
		if (nvstates != _BSEC_SR_NVSTATES_CLOSED) {
			state = BSEC_STATE_INVALID;
			EMSG("BSEC invalid nvstates %#x\n", nvstates);
		} else {
			state = BSEC_STATE_SEC_OPEN;
			if (shadow->value[_OTP_SECURE_BOOT] & _OTP_CLOSED_SECURE)
				state = BSEC_STATE_SEC_CLOSED;
		}
	}

	return state;
}


static void stm32_bsec_shadow_load(const struct device *dev, uint32_t status)
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

	memset(drv_data->p_shadow, 0, sizeof(*drv_data->p_shadow));
	drv_data->p_shadow->magic = BSEC_MAGIC;
	drv_data->p_shadow->state = BSEC_STATE_INVALID;

	//exceptions = bsec_lock();

	/* HIDEUP: read and write not possible in upper region */
	if (status & _BSEC_OTPSR_HIDEUP) {
		for (otp = drv_data->variant->upper_base;
		     otp <= drv_data->variant->max_id ; otp++) {
			drv_data->p_shadow->status[otp] |= _HIDEUP_ERROR;
			drv_data->p_shadow->value[otp] = 0x0U;
		}
		max_id = drv_data->variant->upper_base - 1;
	}

	for (bank = 0U; bank < _OTP_ACCESS_SIZE; bank++) {
		srlock[bank] = io_read32(drv_cfg->base + _BSEC_SRLOCK(bank));
		swlock[bank] = io_read32(drv_cfg->base + _BSEC_SWLOCK(bank));
		splock[bank] = io_read32(drv_cfg->base + _BSEC_SPLOCK(bank));
	}

	for (otp = 0U; otp <= max_id ; otp++) {
		bank = _FLD_GET(_BSEC_OTP_BANK, otp);
		mask = _FLD_GET(_BSEC_OTP_BIT, otp);

		if (srlock[bank] & mask)
			drv_data->p_shadow->status[otp] |= LOCK_SHADOW_R;
		if (swlock[bank] & mask)
			drv_data->p_shadow->status[otp] |= LOCK_SHADOW_W;
		if (splock[bank] & mask)
			drv_data->p_shadow->status[otp] |= LOCK_SHADOW_P;

		if (drv_data->p_shadow->status[otp] & STATUS_SECURE)
			continue;

		/*
		 * OEM keys are accessible only in ROM code
		 * They are stored in last OTPs
		 */
		if (otp >= _OEM_KEY_FIRST_OTP) {
			drv_data->p_shadow->status[otp] |= LOCK_SHADOW_R;
			continue;
		}

		/* reload shadow to read Permanent Programing Lock Flag */
		shadow_otp(dev, otp);

		drv_data->p_shadow->value[otp] = io_read32(drv_cfg->base +
							   _BSEC_FVR(otp));
	}

	//bsec_unlock(exceptions);
}

static void stm32_bsec_mirror_init(const struct device *dev, bool force_load)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);
	struct bsec_shadow *shadow = drv_data->p_shadow;
	uint32_t status;

	status = io_read32(drv_cfg->base + _BSEC_OTPSR);
	stm32_bsec_check_error(status);

	/* update shadow when forced or invalid */
	if (force_load || shadow->magic != BSEC_MAGIC)
		stm32_bsec_shadow_load(dev, status);

	/* always update status */
	shadow->state = init_state(dev, status);
	if ((shadow->state & BSEC_STATE_MASK) == BSEC_STATE_INVALID) {
		EMSG("BSEC invalid state");
		panic();
	}
}

static int stm32_bsec_shadow_init(const struct device *dev)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	const struct nvmem_cell *cell = NULL;
	bool sw_lock;
	int i, j, ret;

	for (i = 0; i < drv_cfg->n_otp_cell; i++) {
		cell = &drv_cfg->otp_cell[i];

		if(cell->n_shadow_value == 0)
			continue;

		/*
		 * The shadow_value array must have a value for all
		 * the OTP of the section.
		 */
		if(cell->n_shadow_value != cell->n_otp){
			EMSG("size of shadow-provisionning not equal to size of"
			     " reg for node otp : %d",
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
			_otp_write(cell->otp_id, cell->n_otp * sizeof(uint32_t),
				   cell->n_shadow_value,
				   (uint8_t*)cell->shadow_value);
		}
	}

	return 0;
}

static int stm32_bsec_dt_init(const struct device *dev)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);

	drv_data->hw_key_valid = false;
	drv_data->p_shadow = (struct bsec_shadow *)drv_cfg->mirror_addr;

	if (IS_ENABLED(STM32_BL2))
		stm32_bsec_mirror_init(dev, true);

	if (drv_data->p_shadow->magic != BSEC_MAGIC)
		return -ENOSYS;

	if (drv_data->p_shadow->state & BSEC_HARDWARE_KEY)
		drv_data->hw_key_valid = true;

	return stm32_bsec_shadow_init(dev);
}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32mp25_bsec)
static struct stm32_bsec_variant variant_stm32mp25 = {
	.upper_base = 256,
	.max_id = 367,
};
#endif

#define NVMEM_CELL_CHILD_DEFINE(node_id)					\
static const uint32_t shadow_value_##node_id[] = 				\
	DT_PROP_OR(node_id, shadow_provisionning, {}); 				\
										\
static const struct nvmem_cell stm32_otp_cell_##node_id = {			\
	.otp_id = (DT_REG_ADDR(node_id) / 4),					\
	.n_otp = (DT_REG_SIZE(node_id) / 4), 					\
	.shadow_value = shadow_value_##node_id,					\
	.n_shadow_value = DT_PROP_LEN_OR(node_id, shadow_provisionning, 0)	\
};

#define NVMEM_CELL_CHILD_GET(node_id) stm32_otp_cell_##node_id,

#define STM32_BSEC3_INIT(node_id, _variant)					\
										\
DT_FOREACH_CHILD(node_id, NVMEM_CELL_CHILD_DEFINE)				\
										\
static const struct nvmem_cell stm32_otp_cells_##node_id [] = {			\
	DT_FOREACH_CHILD(node_id, NVMEM_CELL_CHILD_GET)				\
};										\
										\
static const struct stm32_bsec_config stm32_bsec3_cfg_ ## node_id = {		\
	.base = DT_REG_ADDR(node_id),						\
	.mirror_addr = DT_REG_ADDR(DT_PHANDLE(node_id, memory_region)),		\
	.mirror_size = DT_REG_SIZE(DT_PHANDLE(node_id, memory_region)),		\
	.otp_cell = stm32_otp_cells_##node_id,					\
	.n_otp_cell = ARRAY_SIZE(stm32_otp_cells_##node_id),			\
};										\
										\
static struct stm32_bsec_data stm32_bsec3_data_ ## node_id = {			\
	.variant = _variant,							\
};										\
										\
DEVICE_DT_DEFINE(node_id, &stm32_bsec_dt_init,					\
		 &stm32_bsec3_data_##node_id,					\
		 &stm32_bsec3_cfg_##node_id,					\
		 CORE, 5,							\
		 NULL);

DT_FOREACH_STATUS_OKAY_VARGS(st_stm32mp25_bsec, STM32_BSEC3_INIT,
			     &variant_stm32mp25);
