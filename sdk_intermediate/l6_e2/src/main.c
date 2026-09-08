/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <nrfx_saadc.h>

#define SAADC_INPUT_PIN NRFX_ANALOG_EXTERNAL_AIN4
static nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN, 0);

static int16_t sample;

#define BATTERY_SAMPLE_INTERVAL_MS 2000

static void battery_sample_timer_handler(struct k_timer * timer);

K_TIMER_DEFINE(battery_sample_timer, battery_sample_timer_handler, NULL);

static void battery_sample_work_handler(struct k_work *work)
{
    int err = nrfx_saadc_mode_trigger();
    if (err != 0) {
        printk("nrfx_saadc_mode_trigger error: %08x\n", err);
        return;
    }

    int battery_voltage = ((900 * 4) * sample) / (1 << 12);

    printk("SAADC sample: %d\n", sample);
    printk("Battery Voltage: %d mV\n", battery_voltage);
}

K_WORK_DEFINE(battery_sample_work, battery_sample_work_handler);

void battery_sample_timer_handler(struct k_timer *timer)
{
    k_work_submit(&battery_sample_work);
}

static void configure_saadc(void)
{
	/* Connect ADC interrupt to nrfx interrupt handler */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
				DT_IRQ(DT_NODELABEL(adc), priority),
				nrfx_isr, nrfx_saadc_irq_handler, 0);
        
	int err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
	if (err != 0) {
		printk("nrfx_saadc_mode_trigger error: %08x", err);
		return;
	}

	channel.channel_config.gain = NRF_SAADC_GAIN1_4;
	err = nrfx_saadc_channels_config(&channel, 1);
	if (err != 0) {
		printk("nrfx_saadc_channels_config error: %08x", err);
		return;
	}

	/* Configure nrfx_SAADC driver in simple and blocking mode */
	err = nrfx_saadc_simple_mode_set(BIT(0),
                                 NRF_SAADC_RESOLUTION_12BIT,
                                 NRF_SAADC_OVERSAMPLE_DISABLED,
                                 NULL);
	if (err != 0) {
		printk("nrfx_saadc_simple_mode_set error: %08x", err);
		return;
	}

	err = nrfx_saadc_buffer_set(&sample, 1);
	if (err != 0) {
		printk("nrfx_saadc_buffer_set error: %08x", err);
		return;
	}

	k_timer_start(&battery_sample_timer, K_NO_WAIT, K_MSEC(BATTERY_SAMPLE_INTERVAL_MS));
}

int main(void)
{
	configure_saadc();

	k_sleep(K_FOREVER);
	return 0;
}