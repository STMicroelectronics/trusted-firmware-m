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
#include <lib/utils_def.h>

#include <stm32_bsec3.h>
#include <tfm_plat_otp.h>

/* BSEC REGISTER OFFSET */
#define _BSEC_FVR			U(0x000)
#define _BSEC_SR			U(0xE40)
#define _BSEC_OTPCR			U(0xC04)
#define _BSEC_WDR			U(0xC08)
#define _BSEC_OTPSR			U(0xE44)
#define _BSEC_LOCKR			U(0xE10)
#define _BSEC_DENR			U(0xE20)
#define _BSEC_SFSR			U(0x940)
#define _BSEC_OTPVLDR			U(0x8C0)
#define _BSEC_SPLOCK			U(0x800)
#define _BSEC_SWLOCK			U(0x840)
#define _BSEC_SRLOCK			U(0x880)
#define _BSEC_SCRATCHR0			U(0xE00)
#define _BSEC_SCRATCHR1			U(0xE04)
#define _BSEC_SCRATCHR2			U(0xE08)
#define _BSEC_SCRATCHR3			U(0xE0C)
#define _BSEC_JTAGINR			U(0xE14)
#define _BSEC_JTAGOUTR			U(0xE18)
#define _BSEC_UNMAPR			U(0xE24)
#define _BSEC_WRCR			U(0xF00)
#define _BSEC_HWCFGR			U(0xFF0)
#define _BSEC_VERR			U(0xFF4)
#define _BSEC_IPIDR			U(0xFF8)
#define _BSEC_SIDR			U(0xFFC)

/* BSEC_LOCKR register fields */
#define _BSEC_LOCKR_GWLOCK_MASK		BIT(0)

/* BSEC_DENR register fields */
#define _BSEC_DENR_ALL_MSK		GENMASK(15, 0)

#define _BSEC_DENR_KEY			0xDEB60000

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
};

struct stm32_bsec_data {
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
	uint32_t reserved4[207];
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
 * Interface with TFM
 */
int stm32_bsec_otp_read(enum tfm_otp_element_id_t id,
			size_t out_len, uint8_t *out)
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
	default:
		return -ENOTSUP;
	}

	return 0;
}

int stm32_bsec_otp_size(enum tfm_otp_element_id_t id, size_t *size)
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
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef TFM_DUMMY_PROVISIONING
int stm32_bsec_otp_write(enum tfm_otp_element_id_t id,
			 size_t in_len, const uint8_t *in)
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
int stm32_bsec_otp_write(enum tfm_otp_element_id_t id,
			 size_t in_len, const uint8_t *in)
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

static int stm32_bsec_dt_init(const struct device *dev)
{
	const struct stm32_bsec_config *drv_cfg = dev_get_config(dev);
	struct stm32_bsec_data *drv_data = dev_get_data(dev);

	drv_data->p_shadow = (struct bsec_shadow *)drv_cfg->mirror_addr;
	if (drv_data->p_shadow->magic != BSEC_MAGIC)
		return -ENOSYS;

	drv_data->hw_key_valid = false;
	if (drv_data->p_shadow->state & BSEC_HARDWARE_KEY)
		drv_data->hw_key_valid = true;

	return 0;
}

static const struct stm32_bsec_config _bsec_cfg = {
	.base = DT_INST_REG_ADDR(0),
	.mirror_addr = DT_REG_ADDR(DT_INST_PHANDLE(0, memory_region)),
	.mirror_size = DT_REG_SIZE(DT_INST_PHANDLE(0, memory_region)),
};

static struct stm32_bsec_data _bsec_data;

/*
 * TODO: NVMEM framwork & api
 */
DEVICE_DT_INST_DEFINE(0,
		      &stm32_bsec_dt_init,
		      &_bsec_data, &_bsec_cfg,
		      PRE_CORE, 0,
		      NULL);
