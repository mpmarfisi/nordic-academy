/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

// #define L2_POLLING
#define L2_INTERRUPT

#ifdef L2_POLLING
	#define POLL_TIME_MS   100
#endif

#ifdef L2_INTERRUPT
	#define POLL_TIME_MS   100*60*1000
#endif

#define LED0_NODE DT_ALIAS(led0)
#define SW0_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

#ifdef L2_INTERRUPT
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    gpio_pin_toggle_dt(&led);
}

static struct gpio_callback button_cb_data;
#endif

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	if (!gpio_is_ready_dt(&button)) {
		return -1;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return -1;
	}

	#ifdef L2_INTERRUPT
		ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
		gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin)); 	
		gpio_add_callback(button.port, &button_cb_data);
	#endif

	#ifdef L2_POLLING
		bool val = false;
	#endif

	while (1) {
		#ifdef L2_POLLING
			val = gpio_pin_get_dt(&button);
			gpio_pin_set_dt(&led, val);
		#endif
		k_msleep(POLL_TIME_MS);
	}
	return 0;
}
