/*
 * SPDX-FileCopyrightText: 2010-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_lcd_panel_ops.h"

/* LCD hardware pin configuration for ESP32-S3 */
#define LCD_PIXEL_CLOCK_HZ     (12 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL  1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL
#define PIN_NUM_BK_LIGHT       38
#define PIN_NUM_HSYNC          16
#define PIN_NUM_VSYNC          17
#define PIN_NUM_DE             18
#define PIN_NUM_PCLK           21
#define PIN_NUM_DATA0          4  // B0
#define PIN_NUM_DATA1          5  // B1
#define PIN_NUM_DATA2          6  // B2
#define PIN_NUM_DATA3          7  // B3
#define PIN_NUM_DATA4          15 // B4
#define PIN_NUM_DATA5          8  // G0
#define PIN_NUM_DATA6          20 // G1
#define PIN_NUM_DATA7          3  // G2
#define PIN_NUM_DATA8          46 // G3
#define PIN_NUM_DATA9          9  // G4
#define PIN_NUM_DATA10         10 // G5
#define PIN_NUM_DATA11         11 // R0
#define PIN_NUM_DATA12         12 // R1
#define PIN_NUM_DATA13         13 // R2
#define PIN_NUM_DATA14         14 // R3
#define PIN_NUM_DATA15         0  // R4
#define PIN_NUM_DISP_EN        -1

/* SPI configuration pins */
#define PIN_NUM_SPI_MOSI       47
#define PIN_NUM_SPI_CLK        48
#define PIN_NUM_SPI_CS         39

/* LCD resolution */
#define LCD_H_RES              480
#define LCD_V_RES              480

/* LCD framebuffer configuration */
#if CONFIG_DOUBLE_FB
#define LCD_NUM_FB             2
#else
#define LCD_NUM_FB             1
#endif

/* LVGL tick period */
#define LVGL_TICK_PERIOD_MS    1

/* LCD initialization functions */
esp_lcd_panel_handle_t lcd_init(esp_lcd_rgb_panel_event_callbacks_t *cbs, void *user_data);
void lcd_backlight_init(void);
void lcd_backlight_on(void);

#ifdef __cplusplus
}
#endif

#endif // LCD_CONFIG_H
