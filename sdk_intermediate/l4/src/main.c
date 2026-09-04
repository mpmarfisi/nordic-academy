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

#define PWM_PERIOD_NS 			20000000
#define PWM_PULSE_WIDTH_INITIAL 2000000
#define PWM_STEP_NS 			1000000

#define PWM_LED0    	DT_ALIAS(pwm_led0)
#define BUTTON0			DT_ALIAS(sw0)
#define BUTTON1			DT_ALIAS(sw1)
#define SERVO_MOTOR     DT_NODELABEL(servo) 

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(PWM_LED0);
static const struct pwm_dt_spec pwm_servo = PWM_DT_SPEC_GET(SERVO_MOTOR);
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(BUTTON0, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON1, gpios);

#define PWM_SERVO_MIN_PULSE_WIDTH  DT_PROP(SERVO_MOTOR, min_pulse)
#define PWM_SERVO_MAX_PULSE_WIDTH  DT_PROP(SERVO_MOTOR, max_pulse)

#define PWM_PERIOD   PWM_MSEC(20)

K_EVENT_DEFINE(button_events);

#define BUTTON0_EVENT 0x01
#define BUTTON1_EVENT 0x02

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if(pins & BIT(button0.pin)) {
		k_event_post(&button_events, BUTTON0_EVENT);
	}

	if(pins & BIT(button1.pin)) {
		k_event_post(&button_events, BUTTON1_EVENT);
	}
}

static struct gpio_callback button_cb_data;

int main(void)
{
    int err;
    LOG_INF("Lesson 4 Exercise 1 started");
    
	if (!pwm_is_ready_dt(&pwm_led0)) {
		LOG_ERR("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return 0;
	}

	if (!pwm_is_ready_dt(&pwm_servo)) {
		LOG_ERR("Error: PWM device %s is not ready", pwm_servo.dev->name);
		return 0;
	}

	err = pwm_set_dt(&pwm_servo, PWM_PERIOD, PWM_SERVO_MIN_PULSE_WIDTH);
	if (err) {
		LOG_ERR("pwm_set_dt returned %d", err);
		return 0;
	}

	if (!gpio_is_ready_dt(&button0)) {
		LOG_ERR("Error: button0 device %s is not ready\n", button0.port->name);
		return 0;
	}

	if (!gpio_is_ready_dt(&button1)) {
		LOG_ERR("Error: button1 device %s is not ready\n", button1.port->name);
		return 0;
	}

	err = gpio_pin_configure_dt(&button0, GPIO_INPUT);
	if (err) {
		LOG_ERR("Error in gpio_pin_configure_dt() for button0, err: %d", err);
		return 0;
	}

	err = gpio_pin_configure_dt(&button1, GPIO_INPUT);
	if (err) {
		LOG_ERR("Error in gpio_pin_configure_dt() for button1, err: %d", err);
		return 0;
	}

	// Both buttons are in PORT1
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button0.pin) | BIT(button1.pin)); 	
	err = gpio_add_callback(button0.port, &button_cb_data);
	if(err) {
		LOG_ERR("Error in gpio_add_callback() for button0, err: %d", err);
		return 0;
	}
	
	err = gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_TO_ACTIVE);
	if(err) {
		LOG_ERR("Error in gpio_pin_interrupt_configure_dt() for button0, err: %d", err);
		return 0;
	}

	err = gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
	if(err) {
		LOG_ERR("Error in gpio_pin_interrupt_configure_dt() for button1, err: %d", err);
		return 0;
	}
	
	err = pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, PWM_PULSE_WIDTH_INITIAL);
	if (err) {
		LOG_ERR("Error in pwm_set_dt(), err: %d", err);
		return 0;
	}

	uint32_t events;
	uint32_t pulse_width = PWM_PULSE_WIDTH_INITIAL;

	while(1) {
		// Add a button to increment duty and another one to decrement it
		events = k_event_wait(&button_events, BUTTON0_EVENT | BUTTON1_EVENT, true, K_FOREVER);
		if(events & BUTTON0_EVENT) {
			printk("Button 0 pressed - Incrementing duty cycle\n");
			if(pulse_width <= PWM_PERIOD_NS - PWM_STEP_NS) {
				pulse_width += PWM_STEP_NS;
				printk("PWM pulse width: %u ns\n", pulse_width);
			}
		}
		if(events & BUTTON1_EVENT) {
			printk("Button 1 pressed - Decrementing duty cycle\n");
			if(pulse_width >= PWM_STEP_NS) {
				pulse_width -= PWM_STEP_NS;
				printk("PWM pulse width: %u ns\n", pulse_width);
			}
		}

		err = pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, pulse_width);
		if (err) {
			printk("Error in pwm_set_dt(), err: %d", err);
			continue;
		}
	}

    return 0;
}