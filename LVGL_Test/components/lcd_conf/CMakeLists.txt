idf_component_register(SRCS "lcd_config.c"
                       INCLUDE_DIRS "."
                       REQUIRES "esp_lcd" "driver" "esp_common" "freertos" "log" "espressif__esp_lcd_panel_io_additions" "espressif__esp_lcd_st7701")
