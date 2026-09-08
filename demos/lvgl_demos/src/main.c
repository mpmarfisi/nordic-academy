/*
 * Copyright (c) 2023 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <lvgl_mem.h>
#include <lvgl_zephyr.h>
#include <lv_demos.h>
#include <stdio.h>
#include <zephyr/drivers/mipi_dbi.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

void patch_display_mirroring(void)
{
    const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    
    if (!device_is_ready(display_dev)) {
        return;
    }

    const struct device *dbi_dev = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(ili9341)));

    if (device_is_ready(dbi_dev)) {
        /* 
         * MADCTL (0x36) Bit definitions:
         * MY(0x80) MX(0x40) MV(0x20) ML(0x10) BGR(0x08) MH(0x04)
         * Read your driver's default byte or try forcing the flip:
         * For 90 deg + X-flip, you typically want to toggle the 0x40 or 0x80 bit 
         * depending on your display's orientation.
         */
        uint8_t madctl_val = 0xE0;
        
        struct mipi_dbi_config dbi_config =
            MIPI_DBI_CONFIG_DT(DT_NODELABEL(ili9341),
                               SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0);

        // Write directly to MADCTL (0x36) to change hardware X/Y scanning rules
        mipi_dbi_command_write(dbi_dev, &dbi_config, 0x36, &madctl_val, 1);
    }
}

int main(void)
{
	const struct device *display_dev;
#ifdef CONFIG_LV_Z_DEMO_RENDER_SCENE_DYNAMIC
	k_timepoint_t next_scene_switch;
	lv_demo_render_scene_t cur_scene = LV_DEMO_RENDER_SCENE_FILL;
#endif /* CONFIG_LV_Z_DEMO_RENDER_SCENE_DYNAMIC */
	int ret;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	patch_display_mirroring();

	lvgl_lock();

#if defined(CONFIG_LV_Z_DEMO_MUSIC)
	lv_demo_music();
#elif defined(CONFIG_LV_Z_DEMO_BENCHMARK)
	lv_demo_benchmark();
#elif defined(CONFIG_LV_Z_DEMO_STRESS)
	lv_demo_stress();
#elif defined(CONFIG_LV_Z_DEMO_WIDGETS)
	lv_demo_widgets();
#elif defined(CONFIG_LV_Z_DEMO_KEYPAD_AND_ENCODER)
	lv_demo_keypad_encoder();
#elif defined(CONFIG_LV_Z_DEMO_RENDER)

#ifdef CONFIG_LV_Z_DEMO_RENDER_SCENE_DYNAMIC
	lv_demo_render(cur_scene, 255);
	next_scene_switch =
		sys_timepoint_calc(K_SECONDS(CONFIG_LV_Z_DEMO_RENDER_DYNAMIC_SCENE_TIMEOUT));
#else
	lv_demo_render(CONFIG_LV_Z_DEMO_RENDER_SCENE_INDEX, 255);
#endif /* CONFIG_LV_Z_DEMO_RENDER_SCENE_DYNAMIC */

#else
#error Enable one of the demos CONFIG_LV_Z_DEMO_*
#endif

#ifndef CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE
	lv_timer_handler();
#endif
	lvgl_unlock();

	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return 0;
	}

#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
	lvgl_print_heap_info(false);
#else
	printf("lvgl in malloc mode\n");
#endif
	while (1) {
#ifndef CONFIG_LV_Z_RUN_LVGL_ON_WORKQUEUE
		uint32_t sleep_ms;

		lvgl_lock();
		sleep_ms = lv_timer_handler();
		lvgl_unlock();

		k_msleep(MIN(sleep_ms, INT32_MAX));
#else
		/* LVGL managed by dedicated workqueue, just put an application side delay */
		k_msleep(10);
#endif
#ifdef CONFIG_LV_Z_DEMO_RENDER_SCENE_DYNAMIC
		if (sys_timepoint_expired(next_scene_switch)) {
			cur_scene = (cur_scene + 1) % LV_DEMO_RENDER_SCENE_NUM;
			lvgl_lock();
			lv_demo_render(cur_scene, 255);
			lvgl_unlock();
			next_scene_switch = sys_timepoint_calc(
				K_SECONDS(CONFIG_LV_Z_DEMO_RENDER_DYNAMIC_SCENE_TIMEOUT));
		}
#endif /* CONFIG_LV_Z_DEMO_RENDER_SCENE_DYNAMIC */
	}

	return 0;
}
