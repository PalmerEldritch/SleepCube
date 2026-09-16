#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize the LVGL display and touch integration.
 */
esp_err_t sc_lvgl_display_init(void);

/**
 * @brief Create the current ambient/touch interaction screen.
 */
void sc_lvgl_display_create_ambient_screen(void);

/**
 * @brief Acquire the LVGL/display-side lock before shared SPI storage access.
 *
 * On the Waveshare ESP32-C6 board, the LCD and SD card share the same SPI host.
 * This lock is used by the storage path to serialize SD reads against LVGL flushes.
 *
 * @param timeout_ms Lock timeout in milliseconds.
 * @return `true` if the lock was acquired, otherwise `false`.
 */
bool sc_lvgl_display_bus_lock(uint32_t timeout_ms);

/**
 * @brief Release the shared LVGL/display bus lock.
 */
void sc_lvgl_display_bus_unlock(void);
