/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_chip_info.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "nvs_flash.h"
#include "esp_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_st7701.h"
#include "ui.h"
#include "lcd_config.h"

static const char *TAG = "Main";

#if CONFIG_AVOID_TEAR_EFFECT_WITH_SEM
SemaphoreHandle_t sem_vsync_end;
SemaphoreHandle_t sem_gui_ready;
#endif

static bool on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
#if CONFIG_AVOID_TEAR_EFFECT_WITH_SEM
    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE) {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }
#endif
    return high_task_awoken == pdTRUE;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
#if CONFIG_AVOID_TEAR_EFFECT_WITH_SEM
    xSemaphoreGive(sem_gui_ready);
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
#endif
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    
    lv_disp_flush_ready(drv);
}

static void increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void app_main(void)
{
	ESP_LOGI(TAG, "Free memory: %ld bytes", esp_get_free_heap_size());

	ESP_LOGI(TAG, "ESP-IDF version:%s", esp_get_idf_version());
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	ESP_LOGI(TAG, "chip model is %d, ", chip_info.model);
	ESP_LOGI(TAG, "chip with %d CPU cores, WiFi%s%s",
		chip_info.cores,
		(chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		(chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
	ESP_LOGI(TAG, "silicon revision %d", chip_info.revision);
	uint32_t size_flash_chip;
	esp_flash_get_size(NULL, &size_flash_chip);
	ESP_LOGI(TAG, "%"PRIu32"MB %s flash", size_flash_chip / (1024 * 1024),
			(chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
	static lv_disp_drv_t disp_drv;      // contains callback functions

#if CONFIG_AVOID_TEAR_EFFECT_WITH_SEM
	ESP_LOGI(TAG, "Create semaphores");
	sem_vsync_end = xSemaphoreCreateBinary();
	assert(sem_vsync_end);
	sem_gui_ready = xSemaphoreCreateBinary();
	assert(sem_gui_ready);
#endif

	/* Initialize LCD backlight GPIO */
	lcd_backlight_init();

	/* Initialize LCD panel and get handle */
	esp_lcd_rgb_panel_event_callbacks_t cbs = {
		.on_vsync = on_vsync_event,
	};
	esp_lcd_panel_handle_t panel_handle = lcd_init(&cbs, &disp_drv);

	/* Turn on LCD backlight */
	lcd_backlight_on();


    ESP_LOGI(TAG, "Initialize LVGL library");
	lv_init();
    void *buf1 = NULL;
	void *buf2 = NULL;
#if CONFIG_DOUBLE_FB
	ESP_LOGI(TAG, "Use frame buffers as LVGL draw buffers");
	ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * LCD_V_RES);
#else
	ESP_LOGI(TAG, "Allocate separate LVGL draw buffers from PSRAM");
	buf1 = heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
	assert(buf1);
	buf2 = heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
	assert(buf2);
	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * LCD_V_RES);
#endif

    ESP_LOGI(TAG, "Register display driver to LVGL");
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = LCD_H_RES;
	disp_drv.ver_res = LCD_V_RES;
	disp_drv.flush_cb = lvgl_flush_cb;
	disp_drv.draw_buf = &disp_buf;
	disp_drv.user_data = panel_handle;
#if CONFIG_DOUBLE_FB
	disp_drv.full_refresh = true;
#endif
	lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

	ESP_LOGI(TAG, "Install LVGL tick timer");
	const esp_timer_create_args_t lvgl_tick_timer_args = {
		.callback = &increase_lvgl_tick,
		.name = "lvgl_tick"
	};
	esp_timer_handle_t lvgl_tick_timer = NULL;
	ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

	ESP_LOGI(TAG, "Display LVGL UI");

	
    ui_init();

	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
	    lv_timer_handler();
	}
}
