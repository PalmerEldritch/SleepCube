#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

esp_err_t sc_lcd_panel_if_init(void);
esp_lcd_panel_io_handle_t sc_lcd_panel_if_get_io_handle(void);
esp_lcd_panel_handle_t sc_lcd_panel_if_get_panel_handle(void);
esp_lcd_touch_handle_t sc_lcd_panel_if_get_touch_handle(void);

/**
 * @brief Set the LCD backlight PWM duty directly.
 *
 * Used by the ambient LCD renderer to animate brightness through the panel
 * backlight instead of full-screen pixel color changes.
 *
 * @param level Raw LEDC duty level in the inclusive range 0..1023.
 * @return `ESP_OK` on success, `ESP_ERR_NOT_SUPPORTED` when LCD touch backend
 *         is not compiled for the current board/profile, or another driver
 *         error from the LEDC backend.
 */
esp_err_t sc_lcd_panel_if_set_backlight_level(uint16_t level);

/**
 * @brief Get the currently requested LCD backlight PWM duty.
 *
 * @return Raw LEDC duty level in the inclusive range 0..1023, or 0 when the
 *         LCD backend is not available in the current build.
 */
uint16_t sc_lcd_panel_if_get_backlight_level(void);
