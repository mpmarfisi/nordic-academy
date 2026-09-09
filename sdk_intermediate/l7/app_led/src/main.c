/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <blink.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Lesson7_Exercise3, LOG_LEVEL_INF);

#define BLINK_PERIOD_MS_STEP 100U
#define BLINK_PERIOD_MS_MAX  1000U

int main(void)
{
    /* Start blinking - slow*/ 
    unsigned int period_ms = BLINK_PERIOD_MS_MAX;

    LOG_INF("Zephyr Example Application");

    const struct device * blink[4];
    blink[0]  = DEVICE_DT_GET(DT_NODELABEL(blink_led0));
    blink[1]  = DEVICE_DT_GET(DT_NODELABEL(blink_led1));
    blink[2]  = DEVICE_DT_GET(DT_NODELABEL(blink_led2));
    blink[3]  = DEVICE_DT_GET(DT_NODELABEL(blink_led3));
    
    int ret = 0;
    for (int i = 0; i < 4; i++) {
        if (!device_is_ready(blink[i])) {
            LOG_ERR("Blink LED %d not ready", i);
            return 0;
        }

        /* Use custom API to turn LED off */
        ret = blink_off(blink[i]);
        if (ret < 0) {
            LOG_ERR("Could not turn off LED %d (%d)", i, ret);
            return 0;
        }
    }

    while (1) {
        /* When LED is constantly enabled - start over with high blinking period*/
        if (period_ms == 0U) {
            period_ms = BLINK_PERIOD_MS_MAX;
        } else {
            period_ms -= BLINK_PERIOD_MS_STEP;
        }

        printk("Setting LED period to %u ms\n", period_ms);
        /* Use custom API to change LED blinking period*/
        for (int i = 0; i < 4; i++) {
            ret = blink_set_period_ms(blink[i], period_ms);
            if (ret < 0) {
                LOG_ERR("Could not set LED %d period (%d)", i, ret);
                return 0;
            }
        }
        k_sleep(K_MSEC(2000));
    }

    return 0;
}

