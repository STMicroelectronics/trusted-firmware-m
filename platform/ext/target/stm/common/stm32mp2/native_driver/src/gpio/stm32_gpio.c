/*
 * Copyright (c) 2016-2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include <lib/mmio.h>
#include <lib/utils_def.h>

#ifdef LIBFDT
#include <libfdt.h>
#endif

#ifdef TFM_ENV
#include <debug.h>
#include <stm32_gpio.h>
#include <clk.h>
#else
#include <platform_def.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/st/stm32_gpio.h>
#include <drivers/st/stm32mp_clkfunc.h>
#endif

#define GPIO_MODE_OFFSET	U(0x00)
#define GPIO_TYPE_OFFSET	U(0x04)
#define GPIO_SPEED_OFFSET	U(0x08)
#define GPIO_PUPD_OFFSET	U(0x0C)
#define GPIO_BSRR_OFFSET	U(0x18)
#define GPIO_AFRL_OFFSET	U(0x20)
#define GPIO_AFRH_OFFSET	U(0x24)
#define GPIO_SECR_OFFSET	U(0x30)

#define GPIO_ALTERNATE_MASK		U(0x0F)
#define GPIO_ALT_LOWER_LIMIT	U(0x08)

#define PINMUX_BANK_SHIFT	12
#define PINMUX_BANK_MASK	GENMASK(16, 12)
#define PINMUX_PIN_SHIFT	8
#define PINMUX_PIN_MASK		GENMASK(11, 8)
#define PINMUX_MODE_MASK	GENMASK(7, 0)

static struct stm32_gpio_platdata gpio_pdata;

struct gpio_cfg {
	uint32_t bank;
	uint32_t pin;
	uint32_t mode;
	uint32_t type;
	uint32_t speed;
	uint32_t pull;
	uint32_t alternate;
};

#ifdef LIBFDT
/* LBA TODO; MUST BE review to fill the struct platdata  */
/*******************************************************************************
 * This function gets GPIO bank node in DT.
 * Returns node offset if status is okay in DT, else return 0
 ******************************************************************************/
static int ckeck_gpio_bank(void *fdt, uint32_t bank, int pinctrl_node)
{
	int pinctrl_subnode;
	uint32_t bank_offset = stm32_get_gpio_bank_offset(bank);

	fdt_for_each_subnode(pinctrl_subnode, fdt, pinctrl_node) {
		const fdt32_t *cuint;

		if (fdt_getprop(fdt, pinctrl_subnode,
				"gpio-controller", NULL) == NULL) {
			continue;
		}

		cuint = fdt_getprop(fdt, pinctrl_subnode, "reg", NULL);
		if (cuint == NULL) {
			continue;
		}

		if ((fdt32_to_cpu(*cuint) == bank_offset) &&
		    (fdt_get_status(pinctrl_subnode) != DT_DISABLED)) {
			return pinctrl_subnode;
		}
	}

	return 0;
}

/*******************************************************************************
 * This function gets the pin settings from DT information.
 * When analyze and parsing is done, set the GPIO registers.
 * Returns 0 on success and a negative FDT error code on failure.
 ******************************************************************************/
static int dt_set_gpio_config(void *fdt, int node, uint8_t status)
{
	const fdt32_t *cuint, *slewrate;
	int len;
	int pinctrl_node;
	uint32_t i;
	uint32_t speed = GPIO_SPEED_LOW;
	uint32_t pull = GPIO_NO_PULL;

	cuint = fdt_getprop(fdt, node, "pinmux", &len);
	if (cuint == NULL) {
		return -FDT_ERR_NOTFOUND;
	}

	pinctrl_node = fdt_parent_offset(fdt, fdt_parent_offset(fdt, node));
	if (pinctrl_node < 0) {
		return -FDT_ERR_NOTFOUND;
	}

	slewrate = fdt_getprop(fdt, node, "slew-rate", NULL);
	if (slewrate != NULL) {
		speed = fdt32_to_cpu(*slewrate);
	}

	if (fdt_getprop(fdt, node, "bias-pull-up", NULL) != NULL) {
		pull = GPIO_PULL_UP;
	} else if (fdt_getprop(fdt, node, "bias-pull-down", NULL) != NULL) {
		pull = GPIO_PULL_DOWN;
	} else {
		VERBOSE("No bias configured in node %d\n", node);
	}

	for (i = 0U; i < ((uint32_t)len / sizeof(uint32_t)); i++) {
		uint32_t pincfg;
		uint32_t bank;
		uint32_t pin;
		uint32_t mode;
		uint32_t alternate = GPIO_ALTERNATE_(0);
		int bank_node;
		int clk;

		pincfg = fdt32_to_cpu(*cuint);
		cuint++;

		bank = (pincfg & DT_GPIO_BANK_MASK) >> DT_GPIO_BANK_SHIFT;

		pin = (pincfg & DT_GPIO_PIN_MASK) >> DT_GPIO_PIN_SHIFT;

		mode = pincfg & DT_GPIO_MODE_MASK;

		switch (mode) {
		case 0:
			mode = GPIO_MODE_INPUT;
			break;
		case 1 ... 16:
			alternate = mode - 1U;
			mode = GPIO_MODE_ALTERNATE;
			break;
		case 17:
			mode = GPIO_MODE_ANALOG;
			break;
		default:
			mode = GPIO_MODE_OUTPUT;
			break;
		}

		if (fdt_getprop(fdt, node, "drive-open-drain", NULL) != NULL) {
			mode |= GPIO_OPEN_DRAIN;
		}

		bank_node = ckeck_gpio_bank(fdt, bank, pinctrl_node);
		if (bank_node == 0) {
			ERROR("PINCTRL inconsistent in DT\n");
			panic();
		}

		clk = fdt_get_clock_id(bank_node);
		if (clk < 0) {
			return -FDT_ERR_NOTFOUND;
		}

		/* Platform knows the clock: assert it is okay */
		assert((unsigned long)clk == stm32_get_gpio_bank_clock(bank));

		set_gpio(bank, pin, mode, speed, pull, alternate, status);
	}

	return 0;
}

/*******************************************************************************
 * This function gets the pin settings from DT information.
 * When analyze and parsing is done, set the GPIO registers.
 * Returns 0 on success and a negative FDT/ERRNO error code on failure.
 ******************************************************************************/
int dt_set_pinctrl_config(int node)
{
	const fdt32_t *cuint;
	int lenp = 0;
	uint32_t i;
	uint8_t status = fdt_get_status(node);
	void *fdt;

	if (fdt_get_address(&fdt) == 0) {
		return -FDT_ERR_NOTFOUND;
	}

	if (status == DT_DISABLED) {
		return -FDT_ERR_NOTFOUND;
	}

	cuint = fdt_getprop(fdt, node, "pinctrl-0", &lenp);
	if (cuint == NULL) {
		return -FDT_ERR_NOTFOUND;
	}

	for (i = 0; i < ((uint32_t)lenp / 4U); i++) {
		int p_node, p_subnode;

		p_node = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(*cuint));
		if (p_node < 0) {
			return -FDT_ERR_NOTFOUND;
		}

		fdt_for_each_subnode(p_subnode, fdt, p_node) {
			int ret = dt_set_gpio_config(fdt, p_subnode, status);

			if (ret < 0) {
				return ret;
			}
		}

		cuint++;
	}

	return 0;
}
#else
static int stm32_gpio_parse_fdt(struct stm32_gpio_platdata *pdata)
{
	return -ENOTSUP;
}

__attribute__((weak))
int stm32_gpio_get_platdata(struct stm32_gpio_platdata *pdata)
{
	return -ENODEV;
}

#endif

static const struct bank_cfg *stm32_gpio_get_bankcfg(unsigned int bank_id)
{
	int i;

	for (i = 0; i < gpio_pdata.nbanks; i++) {
		const struct bank_cfg *bank_cfg = gpio_pdata.banks + i;

		if (bank_cfg->id == bank_id)
			return bank_cfg;
	}

	return NULL;
}

static void stm32_pinmux_to_gpio(uint32_t pinmux, struct gpio_cfg *gpio)
{
	gpio->alternate = GPIO_ALTERNATE_(0);
	gpio->bank = (pinmux & PINMUX_BANK_MASK) >> PINMUX_BANK_SHIFT;
	gpio->pin = (pinmux & PINMUX_PIN_MASK) >> PINMUX_PIN_SHIFT;
	gpio->mode = pinmux & PINMUX_MODE_MASK;

	switch (gpio->mode) {
	case 0:
		gpio->mode = GPIO_MODE_INPUT;
		break;
	case 1 ... 16:
		gpio->alternate = gpio->mode - 1U;
		gpio->mode = GPIO_MODE_ALTERNATE;
		break;
	case 17:
		gpio->mode = GPIO_MODE_ANALOG;
		break;
	default:
		gpio->mode = GPIO_MODE_OUTPUT;
		break;
	}
}

static int set_gpio(struct gpio_cfg *gpio)
{
	const struct bank_cfg *bk_cfg;
	unsigned long base;

	bk_cfg = stm32_gpio_get_bankcfg(gpio->bank);
	if (!bk_cfg)
		return -ENOTSUP;

	base = bk_cfg->base;

	clk_enable(bk_cfg->clk_id);

	mmio_clrbits_32(base + GPIO_MODE_OFFSET,
			((uint32_t)GPIO_MODE_MASK << (gpio->pin << 1)));
	mmio_setbits_32(base + GPIO_MODE_OFFSET,
			(gpio->mode & ~GPIO_OPEN_DRAIN) << (gpio->pin << 1));

	if (gpio->type & GPIO_OPEN_DRAIN) {
		mmio_setbits_32(base + GPIO_TYPE_OFFSET, BIT(gpio->pin));
	} else {
		mmio_clrbits_32(base + GPIO_TYPE_OFFSET, BIT(gpio->pin));
	}

	mmio_clrbits_32(base + GPIO_SPEED_OFFSET,
			((uint32_t)GPIO_SPEED_MASK << (gpio->pin << 1)));
	mmio_setbits_32(base + GPIO_SPEED_OFFSET, gpio->speed << (gpio->pin << 1));

	mmio_clrbits_32(base + GPIO_PUPD_OFFSET,
			((uint32_t)GPIO_PULL_MASK << (gpio->pin << 1)));
	mmio_setbits_32(base + GPIO_PUPD_OFFSET, gpio->pull << (gpio->pin << 1));

	if (gpio->pin < GPIO_ALT_LOWER_LIMIT) {
		mmio_clrbits_32(base + GPIO_AFRL_OFFSET,
				((uint32_t)GPIO_ALTERNATE_MASK << (gpio->pin << 2)));
		mmio_setbits_32(base + GPIO_AFRL_OFFSET,
				gpio->alternate << (gpio->pin << 2));
	} else {
		mmio_clrbits_32(base + GPIO_AFRH_OFFSET,
				((uint32_t)GPIO_ALTERNATE_MASK <<
				 ((gpio->pin - GPIO_ALT_LOWER_LIMIT) << 2)));
		mmio_setbits_32(base + GPIO_AFRH_OFFSET,
				gpio->alternate << ((gpio->pin - GPIO_ALT_LOWER_LIMIT) <<
					      2));
	}

	VERBOSE("GPIO %u mode set to 0x%x\n", gpio->bank,
		mmio_read_32(base + GPIO_MODE_OFFSET));
	VERBOSE("GPIO %u speed set to 0x%x\n", gpio->bank,
		mmio_read_32(base + GPIO_SPEED_OFFSET));
	VERBOSE("GPIO %u mode pull to 0x%x\n", gpio->bank,
		mmio_read_32(base + GPIO_PUPD_OFFSET));
	VERBOSE("GPIO %u mode alternate low to 0x%x\n", gpio->bank,
		mmio_read_32(base + GPIO_AFRL_OFFSET));
	VERBOSE("GPIO %u mode alternate high to 0x%x\n", gpio->bank,
		mmio_read_32(base + GPIO_AFRH_OFFSET));

	clk_disable(bk_cfg->clk_id);

	return 0;
}

int set_pinctrl_config(struct pinctrl_cfg *pctrl_cfg)
{
	struct gpio_cfg gpio;
	int i, j, ret;

	if (!pctrl_cfg || !pctrl_cfg->npins )
		return -EINVAL;

	//loop on pins_cfg[i]
	for (i = 0; i < pctrl_cfg->npins; i++) {
		struct pin_cfg* pins_cfg = pctrl_cfg->pins + i;

		/* take common config */
		gpio.speed = pins_cfg->slew_rate;
		gpio.pull = pins_cfg->pull;
		gpio.type = pins_cfg->open_drain;

		//loop on pinsmux table
		for (j = 0; j < pins_cfg->npinsmux; j++) {
			stm32_pinmux_to_gpio(pins_cfg->pinsmux[j], &gpio);
			ret = set_gpio(&gpio);
			if (ret)
				return ret;
		}
	}

	return 0;
}

int stm32_gpio_init(void)
{
	int err;

	err = stm32_gpio_get_platdata(&gpio_pdata);
	if (err)
		return err;

	err = stm32_gpio_parse_fdt(&gpio_pdata);
	if (err && err != -ENOTSUP)
		return err;

	return 0;
}
