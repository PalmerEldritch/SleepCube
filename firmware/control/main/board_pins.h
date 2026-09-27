#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"

/**
 * @brief Default GPIO map for LED strip and temporary button UI on ESP32 DevKitC.
 *
 * These values mirror the defaults in menuconfig (`Kconfig.projbuild`):
 * - LED data: GPIO13
 * - Audio toggle button: GPIO0
 * - Legacy light button: GPIO32 (currently unused in the primary UI model)
 * - Volume up/down buttons: GPIO33 / GPIO21
 * - Brightness up/down buttons: GPIO19 / GPIO18
 *
 * Note: Runtime build uses the CONFIG_SC_* values from sdkconfig. This section
 * documents the default bring-up wiring for quick reference.
 */
#if CONFIG_SC_BOARD_DEVKITC_ESP32
#define SC_LED_STRIP_DATA_GPIO_DEFAULT     GPIO_NUM_13
#define SC_BTN_AUDIO_GPIO_DEFAULT          GPIO_NUM_0
#define SC_BTN_LIGHT_GPIO_DEFAULT          GPIO_NUM_32
#define SC_BTN_VOL_UP_GPIO_DEFAULT         GPIO_NUM_33
#define SC_BTN_VOL_DOWN_GPIO_DEFAULT       GPIO_NUM_21
#define SC_BTN_LIGHT_UP_GPIO_DEFAULT       GPIO_NUM_19
#define SC_BTN_LIGHT_DOWN_GPIO_DEFAULT     GPIO_NUM_18
#endif

/**
 * @brief Waveshare ESP32-C6 touch display pin map.
 *
 * Values are taken from the verified touch-board demo projects in
 * `docs/ESP32-C6-Touch-LCD-1.47-Demo/03_lvgl_example/components/esp_bsp/`.
 */
#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
#define SC_LED_STRIP_DATA_GPIO_DEFAULT     GPIO_NUM_8
#define SC_LCD_SPI_HOST                    SPI2_HOST
#define SC_LCD_DIN_GPIO                    GPIO_NUM_2
#define SC_LCD_MISO_GPIO                   GPIO_NUM_3
#define SC_LCD_CLK_GPIO                    GPIO_NUM_1
#define SC_LCD_CS_GPIO                     GPIO_NUM_14
#define SC_LCD_DC_GPIO                     GPIO_NUM_15
#define SC_LCD_RST_GPIO                    GPIO_NUM_22
#define SC_LCD_BL_GPIO                     GPIO_NUM_23
#define SC_TOUCH_I2C_PORT                  0
#define SC_TOUCH_I2C_SDA_GPIO              GPIO_NUM_18
#define SC_TOUCH_I2C_SCL_GPIO              GPIO_NUM_19
#define SC_TOUCH_RST_GPIO                  GPIO_NUM_20
#define SC_TOUCH_INT_GPIO                  GPIO_NUM_21
#define SC_LCD_H_RES                       172
#define SC_LCD_V_RES                       320
#define SC_SD_CS_GPIO                      GPIO_NUM_4
#else
#define SC_SD_CS_GPIO                      GPIO_NUM_NC
#endif
