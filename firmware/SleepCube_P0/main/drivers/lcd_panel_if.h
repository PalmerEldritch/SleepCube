#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

esp_err_t sc_lcd_panel_if_init(void);
esp_lcd_panel_io_handle_t sc_lcd_panel_if_get_io_handle(void);
esp_lcd_panel_handle_t sc_lcd_panel_if_get_panel_handle(void);
esp_lcd_touch_handle_t sc_lcd_panel_if_get_touch_handle(void);
