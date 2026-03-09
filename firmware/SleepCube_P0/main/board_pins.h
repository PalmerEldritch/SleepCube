#pragma once

#include "driver/gpio.h"

/**
 * @brief ESP32 DevKitC TX pins for I2S audio output.
 *
 * Wiring for MAX98357 test setup:
 * - BCLK: GPIO26
 * - WS/LRCLK: GPIO25
 * - DOUT: GPIO22
 */
#define SC_AUDIO_I2S_BCLK_GPIO   GPIO_NUM_26
#define SC_AUDIO_I2S_WS_GPIO     GPIO_NUM_25
#define SC_AUDIO_I2S_DOUT_GPIO   GPIO_NUM_22

/**
 * @brief RX pins used for external digital loopback verification.
 *
 * Required jumpers:
 * - GPIO26 -> GPIO14 (BCLK)
 * - GPIO25 -> GPIO27 (WS)
 * - GPIO22 -> GPIO34 (DIN)
 */
#define SC_AUDIO_I2S_RX_BCLK_GPIO  GPIO_NUM_14
#define SC_AUDIO_I2S_RX_WS_GPIO    GPIO_NUM_27
#define SC_AUDIO_I2S_RX_DIN_GPIO   GPIO_NUM_34

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
#define SC_LED_STRIP_DATA_GPIO_DEFAULT     GPIO_NUM_13
#define SC_BTN_AUDIO_GPIO_DEFAULT          GPIO_NUM_0
#define SC_BTN_LIGHT_GPIO_DEFAULT          GPIO_NUM_32
#define SC_BTN_VOL_UP_GPIO_DEFAULT         GPIO_NUM_33
#define SC_BTN_VOL_DOWN_GPIO_DEFAULT       GPIO_NUM_21
#define SC_BTN_LIGHT_UP_GPIO_DEFAULT       GPIO_NUM_19
#define SC_BTN_LIGHT_DOWN_GPIO_DEFAULT     GPIO_NUM_18
