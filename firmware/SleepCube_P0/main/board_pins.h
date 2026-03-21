#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"

/**
 * @brief Board-specific TX pins for I2S audio output.
 */
#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
/**
 * Wiring for ESP32-C6 touch display board + mono I2S amplifier:
 * - WS/LRCLK: GPIO5
 * - BCLK: GPIO6
 * - DOUT: GPIO7
 *
 * Amplifier shutdown is tied high in hardware, so no firmware GPIO is used.
 */
#define SC_AUDIO_I2S_BCLK_GPIO   GPIO_NUM_6
#define SC_AUDIO_I2S_WS_GPIO     GPIO_NUM_5
#define SC_AUDIO_I2S_DOUT_GPIO   GPIO_NUM_7
#define SC_AUDIO_AMP_SD_GPIO     GPIO_NUM_NC
#else
/**
 * Wiring for MAX98357 test setup on ESP32 DevKitC:
 * - BCLK: GPIO26
 * - WS/LRCLK: GPIO25
 * - DOUT: GPIO22
 */
#define SC_AUDIO_I2S_BCLK_GPIO   GPIO_NUM_26
#define SC_AUDIO_I2S_WS_GPIO     GPIO_NUM_25
#define SC_AUDIO_I2S_DOUT_GPIO   GPIO_NUM_22
#define SC_AUDIO_AMP_SD_GPIO     GPIO_NUM_NC
#endif

/**
 * @brief RX pins used for external digital loopback verification.
 *
 * Required jumpers:
 * - GPIO26 -> GPIO14 (BCLK)
 * - GPIO25 -> GPIO27 (WS)
 * - GPIO22 -> GPIO34 (DIN)
 */
#if CONFIG_SC_BOARD_DEVKITC_ESP32
#define SC_AUDIO_I2S_RX_BCLK_GPIO  GPIO_NUM_14
#define SC_AUDIO_I2S_RX_WS_GPIO    GPIO_NUM_27
#define SC_AUDIO_I2S_RX_DIN_GPIO   GPIO_NUM_34
#elif CONFIG_SC_BOARD_WAVESHARE_ESP32C6
#define SC_AUDIO_I2S_RX_BCLK_GPIO  GPIO_NUM_NC
#define SC_AUDIO_I2S_RX_WS_GPIO    GPIO_NUM_NC
#define SC_AUDIO_I2S_RX_DIN_GPIO   GPIO_NUM_NC
#else
#define SC_AUDIO_I2S_RX_BCLK_GPIO  GPIO_NUM_NC
#define SC_AUDIO_I2S_RX_WS_GPIO    GPIO_NUM_NC
#define SC_AUDIO_I2S_RX_DIN_GPIO   GPIO_NUM_NC
#endif

/**
 * @brief Default GPIO map for LED strip and temporary button UI on ESP32 DevKitC.
 *
 * These values mirror the defaults in menuconfig (`Kconfig.projbuild`):
 * - LED data: GPIO13
 * - Audio toggle button: GPIO0
 * - Light toggle button: GPIO32
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
#endif
