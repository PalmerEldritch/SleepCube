#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    SC_LED_PIXEL_ORDER_GRB = 0,
    SC_LED_PIXEL_ORDER_RGB = 1,
} sc_led_pixel_order_t;

esp_err_t sc_led_strip_if_init(int gpio_num, size_t led_count, sc_led_pixel_order_t pixel_order);
esp_err_t sc_led_strip_if_write_rgb(const uint8_t *rgb_data, size_t led_count);
esp_err_t sc_led_strip_if_clear(void);
