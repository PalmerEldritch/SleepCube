#pragma once

#include "driver/gpio.h"

// ESP32 DevKitC test wiring to MAX98357:
// BCLK -> GPIO26, LRC/WS -> GPIO25, DIN -> GPIO22
#define SC_AUDIO_I2S_BCLK_GPIO   GPIO_NUM_26
#define SC_AUDIO_I2S_WS_GPIO     GPIO_NUM_25
#define SC_AUDIO_I2S_DOUT_GPIO   GPIO_NUM_22

// External loopback receiver pins (wire TX pins to these with jumpers):
// GPIO26 -> GPIO14 (BCLK), GPIO25 -> GPIO27 (WS), GPIO22 -> GPIO34 (DIN)
#define SC_AUDIO_I2S_RX_BCLK_GPIO  GPIO_NUM_14
#define SC_AUDIO_I2S_RX_WS_GPIO    GPIO_NUM_27
#define SC_AUDIO_I2S_RX_DIN_GPIO   GPIO_NUM_34
