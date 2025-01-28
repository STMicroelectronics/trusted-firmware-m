/*
 * Copyright (c) 2025, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SK_CIPHER_H
#define SK_CIPHER_H

#include <errno.h>
#include <device.h>

#define AES_BLOCK_SIZE_BIT		U(128)
#define AES_BLOCK_SIZE			(AES_BLOCK_SIZE_BIT / U(8))
#define AES_BLOCK_NB_U32		(AES_BLOCK_SIZE / sizeof(uint32_t))
#define AES_KEYSIZE_128			U(16)
#define AES_KEYSIZE_192			U(24)
#define AES_KEYSIZE_256			U(32)
#define AES_IVSIZE			U(16)

/* TODO enum a globalifier et bouger côte sk cipher */
enum sk_cipher_chaining_mode {
	SK_CIPHER_MODE_ECB,
	SK_CIPHER_MODE_CBC,
	SK_CIPHER_MODE_CTR,
	SK_CIPHER_MODE_GCM,
	SK_CIPHER_MODE_CCM,
};

enum sk_cipher_key_selection {
	/* Generic SK CIPHER Key type*/
	SK_CIPHER_KEY_SOFT,
	SK_CIPHER_KEY_HUK,			/* HW unique key */
	/* STM32 SAES specific key type */
	STM32_SAES_KEY_DHU,			/* Derived HW unique key */
	STM32_SAES_KEY_BH,			/* Boot HW key */
	STM32_SAES_KEY_BHU_XOR_BH,		/* XOR of DHUK and BHK */
	STM32_SAES_KEY_WRAPPED,
};

/**
 * @brief AES configuration structure
 * @param is_dec: true if decryption, false if encryption
 * @param ch_mode: define the chaining mode
 * @param key_select: define where the key comes from
 * @param key: pointer to key (if key_select is KEY_SOFT, else unused)
 * @param key_size: key size
 * @param iv: pointer to initialization vector (unused if ch_mode is ECB)
 * @param iv_size: iv size
 */
struct sk_cipher_config_t {
	bool is_dec;
	enum sk_cipher_chaining_mode ch_mode;
	enum sk_cipher_key_selection key_select;
	const void *key;
	size_t key_size;
	void *iv;
	size_t iv_size;
};

/**
 * @brief Allow to recover the sk cipher device according to its node label.
 *
 * Returns a pointer to a device object created from a devicetree node,
 * accordingif to its label, if any device was allocated by a driver.
 *
 * To properly work the device driver must implement the sk cipher api otherwise
 * the call of cipher function may result of a linker error.
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     cipher_label: node@0x0123beef {
 *             compatible = "cp,compatible-cipher";
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     const struct device *dev = SK_CIPHER_GET_BY_NODE_LABEL(cipher_label);
 * @endcode
 *
 * @param label lowercase-and-underscores node label name
 *
 * @return A pointer to the device object created for that node or NULL if the
 * device doesn't exist.
 */
#define SK_CIPHER_GET_BY_NODE_LABEL(label)	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(label))

typedef int (*sk_cipher_ctx_init_t)(const struct device *dev, struct sk_cipher_config_t *config);

typedef int (*sk_cipher_update_t)(const struct device *dev, bool last_block,
			     uint8_t *data_in, uint8_t *data_out,
			     size_t data_size);
struct sk_cipher_driver_api {
	sk_cipher_ctx_init_t ctx_init;
	sk_cipher_update_t update;
};

static const struct device *const skcipher_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(tfm_skcipher));

/**
 * @brief Start an AES computation.
 * @param dev: SAES device
 * @param config: SAES configuration, see sk_cipher_config_t
 * @note this function doesn't access to hardware but stores in driver data the
 * 	 ctx values
 *
 * @retval 0 if OK a standard errno in case of error.
 */
static inline int sk_cipher_ctx_init(const struct device *dev, struct sk_cipher_config_t *config)
{
	const struct sk_cipher_driver_api *api;

	if (!dev)
		dev = skcipher_dev;

	if ((!dev) || !device_is_ready(dev))
		return -ENODEV;

	api = dev->api;

	if (api->ctx_init == NULL)
		return -ENOSYS;

	return api->ctx_init(dev, config);
}

/**
 * @brief Update (or start) an AES de/encrypt process (ECB, CBC or CTR).
 * @param dev: sk cipher device instance
 * @param last_block: true if last payload data block
 * @param data_in: pointer to payload
 * @param data_out: pointer where to save de/encrypted payload
 * @param data_size: payload size
 *
 * @retval 0 if OK a standard errno in case of error.
 */
static inline int sk_cipher_update(const struct device *dev, bool last_block, uint8_t *data_in,
				   uint8_t *data_out, size_t data_len)
{
	const struct sk_cipher_driver_api *api;

	if (!dev)
		dev = skcipher_dev;

	if ((!dev) || !device_is_ready(dev))
		return -ENODEV;

	api = dev->api;

	if (api->update == NULL)
		return -ENOSYS;

	return api->update(dev, last_block, data_in, data_out, data_len);
}

#endif /* SK_CIPHER_H */
