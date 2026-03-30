/*
 * SPDX-FileCopyrightText: 2010-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lcd_config.h"

static const char *TAG = "LCD_CONFIG";

/* ---- ST7701 init parameters ----*/
static const uint8_t ST7701_FF_BK0[] = {0x77,0x01,0x00,0x00,0x10};
static const uint8_t ST7701_C0[]     = {0x3B,0x00};
static const uint8_t ST7701_C1[]     = {0x0D,0x02};
static const uint8_t ST7701_C2[]     = {0x31,0x05};
static const uint8_t ST7701_CD[]     = {0x00};
static const uint8_t ST7701_B0[]     = {0x00,0x11,0x18,0x0E,0x11,0x06,0x07,0x08,0x07,0x22,0x04,0x12,0x0F,0xAA,0x31,0x18};
static const uint8_t ST7701_B1[]     = {0x00,0x11,0x19,0x0E,0x12,0x07,0x08,0x08,0x08,0x22,0x04,0x11,0x11,0xA9,0x32,0x18};

static const uint8_t ST7701_FF_BK1[] = {0x77,0x01,0x00,0x00,0x11};
static const uint8_t ST7701_B0_BK1[] = {0x60};
static const uint8_t ST7701_B1_BK1[] = {0x32};
static const uint8_t ST7701_B2_BK1[] = {0x07};
static const uint8_t ST7701_B3_BK1[] = {0x80};
static const uint8_t ST7701_B5_BK1[] = {0x49};
static const uint8_t ST7701_B7_BK1[] = {0x85};
static const uint8_t ST7701_B8_BK1[] = {0x21};
static const uint8_t ST7701_C1_BK1[] = {0x78};
static const uint8_t ST7701_C2_BK1[] = {0x78};
static const uint8_t ST7701_E0_BK1[] = {0x00,0x1B,0x02};
static const uint8_t ST7701_E1_BK1[] = {0x08,0xA0,0x00,0x00,0x07,0xA0,0x00,0x00,0x00,0x44,0x44};
static const uint8_t ST7701_E2_BK1[] = {0x11,0x11,0x44,0x44,0xED,0xA0,0x00,0x00,0xEC,0xA0,0x00,0x00};
static const uint8_t ST7701_E3_BK1[] = {0x00,0x00,0x11,0x11};
static const uint8_t ST7701_E4_BK1[] = {0x44,0x44};
static const uint8_t ST7701_E5_BK1[] = {0x0A,0xE9,0xD8,0xA0,0x0C,0xEB,0xD8,0xA0,0x0E,0xED,0xD8,0xA0,0x10,0xEF,0xD8,0xA0};
static const uint8_t ST7701_E6_BK1[] = {0x00,0x00,0x11,0x11};
static const uint8_t ST7701_E7_BK1[] = {0x44,0x44};
static const uint8_t ST7701_E8_BK1[] = {0x09,0xE8,0xD8,0xA0,0x0B,0xEA,0xD8,0xA0,0x0D,0xEC,0xD8,0xA0,0x0F,0xEE,0xD8,0xA0};
static const uint8_t ST7701_EB_BK1[] = {0x02,0x00,0xE4,0xE4,0x88,0x00,0x40};
static const uint8_t ST7701_EC_BK1[] = {0x3C,0x00};
static const uint8_t ST7701_ED_BK1[] = {0xAB,0x89,0x76,0x54,0x02,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x20,0x45,0x67,0x98,0xBA};

static const uint8_t ST7701_FF_BK3[] = {0x77,0x01,0x00,0x00,0x13};
static const uint8_t ST7701_E5_BK3[] = {0xE4};

static const uint8_t ST7701_FF_DEF[] = {0x77,0x01,0x00,0x00,0x00};
static const uint8_t ST7701_CMD21[]  = {};
static const uint8_t ST7701_CMD20[]  = {};

static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
	{0xFF, ST7701_FF_BK0, sizeof(ST7701_FF_BK0), 0},
	{0xC0, ST7701_C0, sizeof(ST7701_C0), 0},
	{0xC1, ST7701_C1, sizeof(ST7701_C1), 0},
	{0xC2, ST7701_C2, sizeof(ST7701_C2), 0},
	{0xCD, ST7701_CD, sizeof(ST7701_CD), 0},

	{0xB0, ST7701_B0, sizeof(ST7701_B0), 0},
	{0xB1, ST7701_B1, sizeof(ST7701_B1), 0},

	{0xFF, ST7701_FF_BK1, sizeof(ST7701_FF_BK1), 0},
	{0xB0, ST7701_B0_BK1, sizeof(ST7701_B0_BK1), 0},
	{0xB1, ST7701_B1_BK1, sizeof(ST7701_B1_BK1), 0},
	{0xB2, ST7701_B2_BK1, sizeof(ST7701_B2_BK1), 0},
	{0xB3, ST7701_B3_BK1, sizeof(ST7701_B3_BK1), 0},
	{0xB5, ST7701_B5_BK1, sizeof(ST7701_B5_BK1), 0},
	{0xB7, ST7701_B7_BK1, sizeof(ST7701_B7_BK1), 0},
	{0xB8, ST7701_B8_BK1, sizeof(ST7701_B8_BK1), 0},
	{0xC1, ST7701_C1_BK1, sizeof(ST7701_C1_BK1), 0},
	{0xC2, ST7701_C2_BK1, sizeof(ST7701_C2_BK1), 0},

	{0xE0, ST7701_E0_BK1, sizeof(ST7701_E0_BK1), 0},
	{0xE1, ST7701_E1_BK1, sizeof(ST7701_E1_BK1), 0},
	{0xE2, ST7701_E2_BK1, sizeof(ST7701_E2_BK1), 0},
	{0xE3, ST7701_E3_BK1, sizeof(ST7701_E3_BK1), 0},
	{0xE4, ST7701_E4_BK1, sizeof(ST7701_E4_BK1), 0},
	{0xE5, ST7701_E5_BK1, sizeof(ST7701_E5_BK1), 0},
	{0xE6, ST7701_E6_BK1, sizeof(ST7701_E6_BK1), 0},
	{0xE7, ST7701_E7_BK1, sizeof(ST7701_E7_BK1), 0},
	{0xE8, ST7701_E8_BK1, sizeof(ST7701_E8_BK1), 0},
	{0xEB, ST7701_EB_BK1, sizeof(ST7701_EB_BK1), 0},
	{0xEC, ST7701_EC_BK1, sizeof(ST7701_EC_BK1), 0},
	{0xED, ST7701_ED_BK1, sizeof(ST7701_ED_BK1), 0},

	{0xFF, ST7701_FF_BK3, sizeof(ST7701_FF_BK3), 0},
	{0xE5, ST7701_E5_BK3, sizeof(ST7701_E5_BK3), 0},

	{0xFF, ST7701_FF_DEF, sizeof(ST7701_FF_DEF), 0},
	{0x21, ST7701_CMD21, 0, 0},

	{0x11, NULL, 0, 120},
	{0x29, NULL, 0, 20},
	{0x20, ST7701_CMD20, 0, 0},
};

esp_lcd_panel_handle_t lcd_init(esp_lcd_rgb_panel_event_callbacks_t *cbs, void *user_data)
{
	ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
	spi_line_config_t line_config = {
		.cs_io_type = IO_TYPE_GPIO,
		.cs_gpio_num = PIN_NUM_SPI_CS,
		.scl_io_type = IO_TYPE_GPIO,
		.scl_gpio_num = PIN_NUM_SPI_CLK,
		.sda_io_type = IO_TYPE_GPIO,
		.sda_gpio_num = PIN_NUM_SPI_MOSI,
		.io_expander = NULL,
	};
	esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
	esp_lcd_panel_io_handle_t io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

	ESP_LOGI(TAG, "Install ST7701 panel driver");
	esp_lcd_rgb_panel_config_t rgb_config = {
		.data_width = 16,
		.psram_trans_align = 64,
#if CONFIG_USE_BOUNCE_BUFFER
		.bounce_buffer_size_px = 100 * LCD_H_RES * sizeof(lv_color_t),
#endif
		.clk_src = LCD_CLK_SRC_DEFAULT,
		.disp_gpio_num = PIN_NUM_DISP_EN,
		.pclk_gpio_num = PIN_NUM_PCLK,
		.vsync_gpio_num = PIN_NUM_VSYNC,
		.hsync_gpio_num = PIN_NUM_HSYNC,
		.de_gpio_num = PIN_NUM_DE,
		.data_gpio_nums = {
			PIN_NUM_DATA0,
			PIN_NUM_DATA1,
			PIN_NUM_DATA2,
			PIN_NUM_DATA3,
			PIN_NUM_DATA4,
			PIN_NUM_DATA5,
			PIN_NUM_DATA6,
			PIN_NUM_DATA7,
			PIN_NUM_DATA8,
			PIN_NUM_DATA9,
			PIN_NUM_DATA10,
			PIN_NUM_DATA11,
			PIN_NUM_DATA12,
			PIN_NUM_DATA13,
			PIN_NUM_DATA14,
			PIN_NUM_DATA15,
		},
		.timings = ST7701_480_480_PANEL_60HZ_RGB_TIMING(),
		.flags.fb_in_psram = true,
#if CONFIG_DOUBLE_FB
		.flags.double_fb = true,
#endif
	};

	st7701_vendor_config_t vendor_config = {
		.rgb_config = &rgb_config,
		.init_cmds = lcd_init_cmds,
		.init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
		.flags = {
			.mirror_by_cmd = 1,
			.enable_io_multiplex = 0,
		},
	};

	const esp_lcd_panel_dev_config_t panel_config = {
		.reset_gpio_num = -1,
		.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
		.bits_per_pixel = 18,
		.vendor_config = &vendor_config,
	};

	esp_lcd_panel_handle_t panel_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &panel_handle));

	ESP_LOGI(TAG, "Register event callbacks");
	ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, cbs, user_data));

	ESP_LOGI(TAG, "Initialize RGB LCD panel");
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

	return panel_handle;
}

void lcd_backlight_init(void)
{
#if PIN_NUM_BK_LIGHT >= 0
	ESP_LOGI(TAG, "Turn off LCD backlight");
	gpio_config_t bk_gpio_config = {
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
	};
	ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif
}

void lcd_backlight_on(void)
{
#if PIN_NUM_BK_LIGHT >= 0
	ESP_LOGI(TAG, "Turn on LCD backlight");
	gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
#endif
}

esp_lcd_touch_handle_t touch_init(void)
{
	/* Configure I2C master bus using new driver */
	i2c_master_bus_config_t bus_conf = {
		.i2c_port = TOUCH_I2C_NUM,
		.sda_io_num = TOUCH_I2C_SDA,
		.scl_io_num = TOUCH_I2C_SCL,
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};
	i2c_master_bus_handle_t bus_handle = NULL;
	ESP_ERROR_CHECK(i2c_new_master_bus(&bus_conf, &bus_handle));

	/* Create panel IO for GT911 over I2C */
	esp_lcd_panel_io_handle_t tp_io_handle = NULL;
	esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &tp_io_config, &tp_io_handle));

	/* Initialize GT911 touch controller */
	esp_lcd_touch_config_t tp_cfg = {
		.x_max = LCD_H_RES,
		.y_max = LCD_V_RES,
		.rst_gpio_num = TOUCH_RST_GPIO,
		.int_gpio_num = TOUCH_INT_GPIO,
	};
	esp_lcd_touch_handle_t tp = NULL;
	ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp));

	ESP_LOGI(TAG, "GT911 touch controller initialized");
	return tp;
}
