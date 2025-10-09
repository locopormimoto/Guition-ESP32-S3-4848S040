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
#include "demos/lv_demos.h"

static const char *TAG = "Main";

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
#define PIN_NUM_DISP_EN		   -1

#define PIN_NUM_SPI_MOSI 		47
#define PIN_NUM_SPI_CLK  		48
#define PIN_NUM_SPI_CS   		39

#define LCD_H_RES              480
#define LCD_V_RES              480

#if CONFIG_DOUBLE_FB
#define LCD_NUM_FB             2
#else
#define LCD_NUM_FB             1
#endif

#define LVGL_TICK_PERIOD_MS    1

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

#if PIN_NUM_BK_LIGHT >= 0
	ESP_LOGI(TAG, "Turn off LCD backlight");
	gpio_config_t bk_gpio_config = {
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
	};
	ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

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

	 // ---- ST7701 init parameters ----
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
	esp_lcd_rgb_panel_event_callbacks_t cbs = {
		.on_vsync = on_vsync_event,
	};
	ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));

	ESP_LOGI(TAG, "Initialize RGB LCD panel");
	ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
	ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

#if PIN_NUM_BK_LIGHT >= 0
	ESP_LOGI(TAG, "Turn on LCD backlight");
	gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
#endif

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
	lv_demo_music();
    
	while (1)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
	    lv_timer_handler();
	}
}
