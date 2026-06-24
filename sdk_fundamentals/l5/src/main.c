/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

#define SLEEP_TIME_MS   1000
#define RECEIVE_BUFF_SIZE 10
#define RECEIVE_TIMEOUT 100

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

const struct device *uart= DEVICE_DT_GET(DT_NODELABEL(uart20));

static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};

static uint8_t tx_buf[] =   {"nRF Connect SDK Fundamentals Course\r\n"
                             "Press 0-2 on your keyboard to toggle LEDS 0-2 on your development kit\r\n"};

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {

	case UART_RX_RDY:
		if ((evt->data.rx.len) == 2) {
			if (evt->data.rx.buf[evt->data.rx.offset] == '0') {
				gpio_pin_toggle_dt(&led0);
			} else if (evt->data.rx.buf[evt->data.rx.offset] == '1') {
				gpio_pin_toggle_dt(&led1);
			} else if (evt->data.rx.buf[evt->data.rx.offset] == '2') {
				gpio_pin_toggle_dt(&led2);
			}
		}
		break;
	case UART_RX_DISABLED:
		uart_rx_enable(dev, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
		break;

	default:
		break;
	}
}

int main(void)
{
	int ret;

	if (!device_is_ready(led0.port)){
		printk("GPIO device is not ready\r\n");
		return 1;
	}

	if (!device_is_ready(uart)){
		printk("UART device not ready\r\n");
		return 1 ;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 1 ; 
	}
	ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 1 ;
	}
	ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 1 ;
	}

	ret = uart_callback_set(uart, uart_cb, NULL);
	if (ret) {
		return 1;
	}

	ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_US);
	if (ret) {
		return 1;
	}

	ret = uart_rx_enable(uart ,rx_buf,sizeof(rx_buf),RECEIVE_TIMEOUT);
	if (ret) {
		return 1;
	}

	while (1) {
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
