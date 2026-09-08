/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Lesson6_Exercise3, LOG_LEVEL_DBG);

#include <nrfx_saadc.h>
#include <nrfx_timer.h>
#include <helpers/nrfx_gppi.h>

#define SAADC_SAMPLE_INTERVAL_US 50
#define SAADC_BUFFER_SIZE 8000
#define SAADC_INPUT_PIN NRFX_ANALOG_EXTERNAL_AIN4
static nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(SAADC_INPUT_PIN, 0);

#define TIMER_INSTANCE_NUMBER NRF_TIMER22
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_NUMBER);

static int16_t saadc_sample_buffer[2][SAADC_BUFFER_SIZE];
static uint32_t saadc_current_buffer = 0;

static void configure_timer(void)
{
    int err;

	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(1000000);
	err = nrfx_timer_init(&timer_instance, &timer_config, NULL);
	if (err != 0) {
		LOG_ERR("nrfx_timer_init error: %08x", err);
		return;
	}

	uint32_t timer_ticks = nrfx_timer_us_to_ticks(&timer_instance, SAADC_SAMPLE_INTERVAL_US);
	nrfx_timer_extended_compare(&timer_instance, NRF_TIMER_CC_CHANNEL0, timer_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
}

static void saadc_event_handler(nrfx_saadc_evt_t const * p_event)
{
    int err;
    switch (p_event->type)
    {
        case NRFX_SAADC_EVT_READY:
			nrfx_timer_enable(&timer_instance);
            break;                        
            
        case NRFX_SAADC_EVT_BUF_REQ:
			err = nrfx_saadc_buffer_set(saadc_sample_buffer[(saadc_current_buffer++)%2], SAADC_BUFFER_SIZE);
			if (err != 0) {
				LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
				return;
			}
            break;

        case NRFX_SAADC_EVT_DONE:
			int64_t average = 0;
			int16_t max = INT16_MIN;
			int16_t min = INT16_MAX;
			int16_t current_value;
			for (int i = 0; i < p_event->data.done.size; i++) {
				current_value = ((int16_t *)(p_event->data.done.p_buffer))[i];
				average += current_value;
				if (current_value > max) {
					max = current_value;
				}
				if (current_value < min) {
					min = current_value;
				}
			}
			average = average / p_event->data.done.size;
			LOG_INF("SAADC buffer at 0x%x filled with %d samples", (uint32_t)p_event->data.done.p_buffer,
				p_event->data.done.size);
			LOG_INF("AVG=%d, MIN=%d, MAX=%d", (int16_t)average, min, max);

            break;

        default:
            LOG_INF("Unhandled SAADC evt %d", p_event->type);
            break;
    }
}

static void configure_saadc(void)
{
    int err;

	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
            	DT_IRQ(DT_NODELABEL(adc), priority),
            	nrfx_isr, nrfx_saadc_irq_handler, 0);

	err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
	if (err != 0) {
		LOG_ERR("nrfx_saadc_init error: %08x", err);
		return;
	}

	channel.channel_config.gain = NRF_SAADC_GAIN1_4;
	err = nrfx_saadc_channels_config(&channel, 1);
	if (err != 0) {
		LOG_ERR("nrfx_saadc_channels_config error: %08x", err);
		return;
	}

	nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
	err = nrfx_saadc_advanced_mode_set(BIT(0),
										NRF_SAADC_RESOLUTION_12BIT,
										&saadc_adv_config,
										saadc_event_handler);
	if (err != 0) {
		LOG_ERR("nrfx_saadc_advanced_mode_set error: %08x", err);
		return;
	}

	err = nrfx_saadc_buffer_set(saadc_sample_buffer[0], SAADC_BUFFER_SIZE);
	if (err != 0) {
		LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
		return;
	}
	err = nrfx_saadc_buffer_set(saadc_sample_buffer[1], SAADC_BUFFER_SIZE);
	if (err != 0) {
		LOG_ERR("nrfx_saadc_buffer_set error: %08x", err);
		return;
	}

    /* This will not start sampling, but will prepare buffer for sampling triggered through PPI */
	err = nrfx_saadc_mode_trigger();
	if (err != 0) {
		LOG_ERR("nrfx_saadc_mode_trigger error: %08x", err);
		return;
	}
}

static void configure_ppi(void)
{
    int err;
    
	nrfx_gppi_handle_t gppi_handle_sample;
	nrfx_gppi_handle_t gppi_handle_start;

    /* Trigger task sample from timer */
  	err = nrfx_gppi_conn_alloc(nrfx_timer_compare_event_address_get(&timer_instance, NRF_TIMER_CC_CHANNEL0),
                               nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE), &gppi_handle_sample);
    if (err != 0) {
        LOG_ERR("nrfx_gppi_conn_alloc error: %08x", err);
        return;
    }
    /* Trigger task start from end event */
	err = nrfx_gppi_conn_alloc(nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END),
                               nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START), &gppi_handle_start);
    if (err != 0) {
        LOG_ERR("nrfx_gppi_conn_alloc error: %08x", err);
        return;
    }
    
	nrfx_gppi_conn_enable(gppi_handle_sample);
	nrfx_gppi_conn_enable(gppi_handle_start);
}


int main(void)
{
    configure_timer();
    configure_saadc();  
    configure_ppi();
    k_sleep(K_FOREVER);
}