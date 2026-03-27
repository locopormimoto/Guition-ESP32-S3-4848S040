/*
 * SPDX-FileCopyrightText: 2010-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#ifndef LCD_LVGL_CALLBACKS_H
#define LCD_LVGL_CALLBACKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl.h"

/* LVGL and LCD synchronization semaphores */
#if CONFIG_AVOID_TEAR_EFFECT_WITH_SEM
extern SemaphoreHandle_t sem_vsync_end;
extern SemaphoreHandle_t sem_gui_ready;

/**
 * @brief Initialize LVGL synchronization semaphores
 * 
 * Creates the binary semaphores used for VSYNC synchronization.
 * Must be called once before using the LVGL callbacks.
 */
void lvgl_sync_init(void);
#endif

/**
 * @brief VSYNC event callback for RGB panel
 * 
 * Handles synchronization between LVGL and LCD refresh to avoid tear effects.
 * This callback is triggered on every vertical sync event from the LCD panel.
 */
bool on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data);

/**
 * @brief LVGL flush callback
 * 
 * Called by LVGL when it needs to render content to the display.
 * Handles drawing the bitmap to the LCD panel with optional tearing effect prevention.
 */
void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);

/**
 * @brief LVGL tick increment callback
 * 
 * Called periodically by a timer to update LVGL's internal timing.
 * Increases LVGL's tick by LVGL_TICK_PERIOD_MS milliseconds.
 */
void increase_lvgl_tick(void *arg);

#ifdef __cplusplus
}
#endif

#endif // LCD_LVGL_CALLBACKS_H
