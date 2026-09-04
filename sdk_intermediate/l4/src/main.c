/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
 
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(Lesson4_Exercise1, LOG_LEVEL_INF);

#define PWM_PERIOD_NS 20000000
#define PWM_PULSE_WIDTH 1400000

#define PWM_LED0    DT_ALIAS(pwm_led0)

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(PWM_LED0);

int main(void)
{
    int err;
    LOG_INF("Lesson 4 Exercise 1 started");
    
	if (!pwm_is_ready_dt(&pwm_led0)) {
		LOG_ERR("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return 0;
	}

	err = pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, PWM_PULSE_WIDTH);
	if (err) {
		LOG_ERR("Error in pwm_set_dt(), err: %d", err);
		return 0;
	}

    return 0;
}