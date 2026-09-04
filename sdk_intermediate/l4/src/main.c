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

#define PWM_LED_PERIOD_NS 				20000000
#define PWM_LED_PULSE_WIDTH_INITIAL 	2000000
#define PWM_LED_STEP_NS 				1000000

#define PWM_SERVO_MIN_PULSE_WIDTH  		DT_PROP(SERVO_MOTOR, min_pulse)
#define PWM_SERVO_MAX_PULSE_WIDTH  		DT_PROP(SERVO_MOTOR, max_pulse)
#define PWM_SERVO_PERIOD   				PWM_MSEC(20)

#define PWM_LED0    					DT_ALIAS(pwm_led0)
#define SERVO_MOTOR     				DT_NODELABEL(servo) 

#define BUTTON0							DT_ALIAS(sw0)
#define BUTTON1							DT_ALIAS(sw1)
#define BUTTON2							DT_ALIAS(sw2)
#define BUTTON3							DT_ALIAS(sw3)

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(PWM_LED0);
static const struct pwm_dt_spec pwm_servo = PWM_DT_SPEC_GET(SERVO_MOTOR);
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(BUTTON0, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(BUTTON1, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(BUTTON2, gpios);
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET(BUTTON3, gpios);

K_EVENT_DEFINE(button_events);

#define BUTTON0_EVENT 0x01
#define BUTTON1_EVENT 0x02
#define BUTTON2_EVENT 0x04
#define BUTTON3_EVENT 0x08

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if(pins & BIT(button0.pin)) {
		k_event_post(&button_events, BUTTON0_EVENT);
	}

	if(pins & BIT(button1.pin)) {
		k_event_post(&button_events, BUTTON1_EVENT);
	}

	if(pins & BIT(button2.pin)) {
		k_event_post(&button_events, BUTTON2_EVENT);
	}

	if(pins & BIT(button3.pin)) {
		k_event_post(&button_events, BUTTON3_EVENT);
	}
}

static struct gpio_callback button012_cb_data;
static struct gpio_callback button3_cb_data;

static int buttons_init(void){
	int err;
	const struct gpio_dt_spec * buttons[] = {&button0, &button1, &button2, &button3};	

	for(int i = 0; i < ARRAY_SIZE(buttons); i++) {
		if (!gpio_is_ready_dt(buttons[i])) {
			LOG_ERR("Error: button%d device %s is not ready\n", i, buttons[i]->port->name);
			return 0;
		}

		err = gpio_pin_configure_dt(buttons[i], GPIO_INPUT);
		if (err) {
			LOG_ERR("Error in gpio_pin_configure_dt() for button%d, err: %d", i, err);
			return 0;
		}

		err = gpio_pin_interrupt_configure_dt(buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
		if(err) {
			LOG_ERR("Error in gpio_pin_interrupt_configure_dt() for button%d, err: %d", i, err);
			return 0;
		}
	}

	// If button 3 that is on another port shared pin we should add a check on the handler for the port
	gpio_init_callback(&button012_cb_data, button_pressed, BIT(button0.pin) | BIT(button1.pin) | BIT(button2.pin)); 	
	err = gpio_add_callback(button0.port, &button012_cb_data);
	if(err) {
		LOG_ERR("Error in gpio_add_callback() for button012, err: %d", err);
		return 0;
	}
	// err = gpio_add_callback(button1.port, &button012_cb_data); // Port shared with button 0
	// err = gpio_add_callback(button2.port, &button012_cb_data); // Port shared with button 0
	gpio_init_callback(&button3_cb_data, button_pressed, BIT(button3.pin)); 	
	err = gpio_add_callback(button3.port, &button3_cb_data);
	if(err) {
		LOG_ERR("Error in gpio_add_callback() for button3, err: %d", err);
		return 0;
	}

	return 0;
}

static int set_motor_angle(uint32_t pulse_width_ns)
{
    int err;
    
    err = pwm_set_dt(&pwm_servo, PWM_SERVO_PERIOD, pulse_width_ns);
    if (err) {
        LOG_ERR("pwm_set_dt_returned %d", err);
    }
    return err;
}

static int set_led_duty_cycle(uint32_t pulse_width_ns)
{
	int err;
	err = pwm_set_dt(&pwm_led0, PWM_LED_PERIOD_NS, pulse_width_ns);
	if (err) {
		LOG_ERR("Error in pwm_set_dt(), err: %d", err);
	}
	return err;
}

int main(void)
{
    int err;
    LOG_INF("Lesson 4 Exercise 1 started");
    
	if(buttons_init() != 0) {
		LOG_ERR("Error initializing buttons");
		return 0;
	}

	if (!pwm_is_ready_dt(&pwm_led0)) {
		LOG_ERR("Error: PWM device %s is not ready\n", pwm_led0.dev->name);
		return 0;
	}

	if (!pwm_is_ready_dt(&pwm_servo)) {
		LOG_ERR("Error: PWM device %s is not ready", pwm_servo.dev->name);
		return 0;
	}

	err = pwm_set_dt(&pwm_servo, PWM_SERVO_PERIOD, PWM_SERVO_MIN_PULSE_WIDTH);
	if (err) {
		LOG_ERR("pwm_set_dt returned %d", err);
		return 0;
	}
	
	err = pwm_set_dt(&pwm_led0, PWM_LED_PERIOD_NS, PWM_LED_PULSE_WIDTH_INITIAL);
	if (err) {
		LOG_ERR("Error in pwm_set_dt(), err: %d", err);
		return 0;
	}

	uint32_t events;
	uint32_t pulse_width = PWM_LED_PULSE_WIDTH_INITIAL;

	while(1) {
		// Add a button to increment duty and another one to decrement it
		events = k_event_wait(&button_events, BUTTON0_EVENT | BUTTON1_EVENT | BUTTON2_EVENT | BUTTON3_EVENT, true, K_FOREVER);
		if(events & BUTTON0_EVENT) {
			printk("Button 0 pressed - Incrementing duty cycle\n");
			if(pulse_width <= PWM_LED_PERIOD_NS - PWM_LED_STEP_NS) {
				pulse_width += PWM_LED_STEP_NS;
				printk("PWM pulse width: %u ns\n", pulse_width);
				set_led_duty_cycle(pulse_width);
			}
			
		}

		if(events & BUTTON1_EVENT) {
			printk("Button 1 pressed - Decrementing duty cycle\n");
			if(pulse_width >= PWM_LED_STEP_NS) {
				pulse_width -= PWM_LED_STEP_NS;
				printk("PWM pulse width: %u ns\n", pulse_width);
				set_led_duty_cycle(pulse_width);
			}
		}

		if(events & BUTTON2_EVENT) {
			printk("Button 2 pressed - Setting servo to min angle\n");
			set_motor_angle(PWM_SERVO_MIN_PULSE_WIDTH);
		}
		
		if(events & BUTTON3_EVENT) {
			printk("Button 3 pressed - Setting servo to max angle\n");
			set_motor_angle(PWM_SERVO_MAX_PULSE_WIDTH);
		}

	}

    return 0;
}