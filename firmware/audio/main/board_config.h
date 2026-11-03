#pragma once

#include "driver/gpio.h"
#include "driver/i2s_std.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "SleepCube Audio Controller supports only the AtomS3 Lite / ESP32-S3 target"
#endif

/* AtomS3 Lite to ATOMIC Speaker Base connector mapping. */
#define SC_AUDIO_I2S_PORT             I2S_NUM_0
#define SC_AUDIO_I2S_BCLK_GPIO        GPIO_NUM_5
#define SC_AUDIO_I2S_LRCLK_GPIO       GPIO_NUM_39
#define SC_AUDIO_I2S_DATA_GPIO        GPIO_NUM_38
#define SC_BOARD_BUTTON_GPIO          GPIO_NUM_41

/* ATOMIC Speaker Base microSD wiring. Card select is tied low on the base. */
#define SC_SDCARD_SPI_HOST            SPI2_HOST
#define SC_SDCARD_MOSI_GPIO           GPIO_NUM_6
#define SC_SDCARD_MISO_GPIO           GPIO_NUM_8
#define SC_SDCARD_CLK_GPIO            GPIO_NUM_7

/* Confirmed amplifier-facing format from M1. */
#define SC_AUDIO_SAMPLE_RATE_HZ       44100U
#define SC_AUDIO_BITS_PER_SAMPLE      16U
#define SC_AUDIO_CHANNEL_COUNT        2U
#define SC_AUDIO_BYTES_PER_FRAME      \
    (SC_AUDIO_CHANNEL_COUNT * (SC_AUDIO_BITS_PER_SAMPLE / 8U))
#define SC_AUDIO_DMA_DESC_NUM         6U
#define SC_AUDIO_DMA_FRAME_NUM        256U
#define SC_AUDIO_DMA_FRAME_NUM_24BIT  255U
#define SC_AUDIO_WRITE_TIMEOUT_MS     1000U
#define SC_AUDIO_FORMAT_NAME          "Philips-I2S stereo signed-16, duplicated mono"

#define SC_STARTUP_LOG_DELAY_MS       2000U
#define SC_BUTTON_DEBOUNCE_MS         30U
#define SC_BUTTON_POLL_MS             10U
#define SC_BUTTON_LONG_PRESS_MS       1000U
