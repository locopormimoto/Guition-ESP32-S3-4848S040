/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_lcd_touch_gt911.h"
#include "driver/i2c.h"

static const char *TAG = "Main";

#define PIN_NUM_I2C_SDA 		19
#define PIN_NUM_I2C_SCL  		45
#define PIN_NUM_TOUCH_RST  		-1
#define PIN_NUM_TOUCH_INT 		-1

#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      400000

#define LCD_H_RES               480
#define LCD_V_RES               480

uint16_t touch_x[1];
uint16_t touch_y[1];
uint16_t touch_strength[1];
uint8_t touch_cnt = 0;

void app_main(void)
{
    esp_lcd_touch_handle_t touch_handle = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = tp_io_config.dev_addr,
    };

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_NUM_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = PIN_NUM_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = PIN_NUM_TOUCH_RST,
        .int_gpio_num = PIN_NUM_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .driver_data = &tp_gt911_config,
    };

    esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM, &tp_io_config, &tp_io_handle);
    esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);

    while(1)
    {
        esp_lcd_touch_read_data(touch_handle);

        bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, touch_x, touch_y, touch_strength, &touch_cnt, 1);

        if (touchpad_pressed)
        {
            ESP_LOGI(TAG, "%d %d", touch_x[0], touch_y[0]);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}