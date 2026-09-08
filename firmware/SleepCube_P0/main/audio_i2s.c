#include "audio_i2s.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "esp_check.h"
#include "esp_log.h"
#include "soc/soc_caps.h"
#include "board_pins.h"

static const char *TAG = "sc_audio_i2s";
#if CONFIG_SC_AUDIO_I2S_MODE_TDM8
#define SC_AUDIO_TDM_SLOT_COUNT 8U
#define SC_AUDIO_TDM_WRITE_FRAMES 128U
#endif

static i2s_chan_handle_t s_tx_chan;
#if (SOC_I2S_NUM > 1)
static i2s_chan_handle_t s_rx_chan;
#endif
static bool s_tx_enabled;
#if CONFIG_SC_AUDIO_I2S_MODE_TDM8
static int16_t s_tdm_write_buf[SC_AUDIO_TDM_WRITE_FRAMES * SC_AUDIO_TDM_SLOT_COUNT];
#endif

static uint64_t sc_gpio_pin_mask(gpio_num_t gpio_num)
{
    if ((gpio_num < 0) || (gpio_num >= 64)) {
        return 0;
    }
    return 1ULL << (uint32_t)gpio_num;
}

static i2s_std_slot_config_t sc_audio_i2s_tx_slot_config(void)
{
#if CONFIG_SC_AUDIO_I2S_FRAMING_PHILIPS
    i2s_std_slot_config_t slot_cfg =
        (i2s_std_slot_config_t)I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
#else
    i2s_std_slot_config_t slot_cfg =
        (i2s_std_slot_config_t)I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
#endif
    slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    return slot_cfg;
}

static const char *sc_audio_i2s_tx_framing_name(void)
{
#if CONFIG_SC_AUDIO_I2S_FRAMING_PHILIPS
    return "philips";
#else
    return "msb";
#endif
}

#if CONFIG_SC_AUDIO_I2S_MODE_TDM8
static i2s_tdm_slot_config_t sc_audio_i2s_tx_tdm_slot_config(void)
{
    const i2s_tdm_slot_mask_t slot_mask = I2S_TDM_SLOT0 |
                                          I2S_TDM_SLOT1 |
                                          I2S_TDM_SLOT2 |
                                          I2S_TDM_SLOT3 |
                                          I2S_TDM_SLOT4 |
                                          I2S_TDM_SLOT5 |
                                          I2S_TDM_SLOT6 |
                                          I2S_TDM_SLOT7;
#if CONFIG_SC_AUDIO_I2S_FRAMING_PHILIPS
    i2s_tdm_slot_config_t slot_cfg =
        I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, slot_mask);
#else
    i2s_tdm_slot_config_t slot_cfg =
        I2S_TDM_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, slot_mask);
#endif
    slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    slot_cfg.total_slot = SC_AUDIO_TDM_SLOT_COUNT;
    return slot_cfg;
}
#endif

static esp_err_t sc_audio_i2s_init_tx(uint32_t sample_rate_hz)
{
    if (SC_AUDIO_AMP_SD_GPIO != GPIO_NUM_NC) {
        const gpio_num_t amp_sd_gpio = SC_AUDIO_AMP_SD_GPIO;
        const gpio_config_t amp_sd_cfg = {
            .pin_bit_mask = sc_gpio_pin_mask(amp_sd_gpio),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&amp_sd_cfg), TAG, "amp sd gpio config failed");
        ESP_RETURN_ON_ERROR(gpio_set_level(amp_sd_gpio, 1), TAG, "amp sd gpio set failed");
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 12;
    chan_cfg.dma_frame_num = 512;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx_chan, NULL), TAG, "i2s_new_channel failed");

#if CONFIG_SC_AUDIO_I2S_MODE_TDM8
    i2s_tdm_slot_config_t slot_cfg = sc_audio_i2s_tx_tdm_slot_config();

    i2s_tdm_config_t tdm_cfg = {
        .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(sample_rate_hz),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = SC_AUDIO_I2S_BCLK_GPIO,
            .ws = SC_AUDIO_I2S_WS_GPIO,
            .dout = SC_AUDIO_I2S_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_tdm_mode(s_tx_chan, &tdm_cfg), TAG, "i2s_channel_init_tdm_mode failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx_chan), TAG, "i2s_channel_enable failed");
    s_tx_enabled = true;
    ESP_LOGI(TAG, "tx format active: tdm8-%s-32 (mono mix in all slots)", sc_audio_i2s_tx_framing_name());
#else
    i2s_std_slot_config_t slot_cfg = sc_audio_i2s_tx_slot_config();

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate_hz),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = SC_AUDIO_I2S_BCLK_GPIO,
            .ws = SC_AUDIO_I2S_WS_GPIO,
            .dout = SC_AUDIO_I2S_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx_chan, &std_cfg), TAG, "i2s_channel_init_std_mode failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx_chan), TAG, "i2s_channel_enable failed");
    s_tx_enabled = true;
    ESP_LOGI(TAG, "tx format active: %s-32", sc_audio_i2s_tx_framing_name());
#endif
    return ESP_OK;
}

esp_err_t sc_audio_i2s_init(uint32_t sample_rate_hz)
{
    return sc_audio_i2s_init_tx(sample_rate_hz);
}

esp_err_t sc_audio_i2s_init_rx(uint32_t sample_rate_hz)
{
#if (SOC_I2S_NUM <= 1)
    (void)sample_rate_hz;
    return ESP_ERR_NOT_SUPPORTED;
#elif (SC_AUDIO_I2S_RX_BCLK_GPIO == GPIO_NUM_NC) || (SC_AUDIO_I2S_RX_WS_GPIO == GPIO_NUM_NC) || (SC_AUDIO_I2S_RX_DIN_GPIO == GPIO_NUM_NC)
    (void)sample_rate_hz;
    return ESP_ERR_NOT_SUPPORTED;
#else
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_SLAVE);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &s_rx_chan), "sc_audio_i2s", "i2s_new_channel rx failed");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate_hz),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = SC_AUDIO_I2S_RX_BCLK_GPIO,
            .ws = SC_AUDIO_I2S_RX_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = SC_AUDIO_I2S_RX_DIN_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_rx_chan, &std_cfg), "sc_audio_i2s", "i2s_channel_init_std_mode rx failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_rx_chan), "sc_audio_i2s", "i2s_channel_enable rx failed");
    return ESP_OK;
#endif
}

esp_err_t sc_audio_i2s_write(const int16_t *stereo_pcm, size_t frame_count, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(stereo_pcm != NULL, ESP_ERR_INVALID_ARG, TAG, "null stereo pcm");
    ESP_RETURN_ON_FALSE(s_tx_enabled, ESP_ERR_INVALID_STATE, TAG, "tx channel disabled");

#if CONFIG_SC_AUDIO_I2S_MODE_TDM8
    size_t frame_offset = 0;
    while (frame_offset < frame_count) {
        size_t frames_this_write = frame_count - frame_offset;
        if (frames_this_write > SC_AUDIO_TDM_WRITE_FRAMES) {
            frames_this_write = SC_AUDIO_TDM_WRITE_FRAMES;
        }

        for (size_t frame = 0; frame < frames_this_write; frame++) {
            const size_t stereo_index = (frame_offset + frame) * 2U;
            const int16_t left = stereo_pcm[stereo_index + 0U];
            const int16_t right = stereo_pcm[stereo_index + 1U];
            const int16_t mono = (int16_t)(((int32_t)left + (int32_t)right) / 2);
            for (size_t slot = 0; slot < SC_AUDIO_TDM_SLOT_COUNT; slot++) {
                s_tdm_write_buf[(frame * SC_AUDIO_TDM_SLOT_COUNT) + slot] = mono;
            }
        }

        size_t bytes_written = 0;
        const size_t bytes_to_write = frames_this_write * SC_AUDIO_TDM_SLOT_COUNT * sizeof(int16_t);
        esp_err_t err = i2s_channel_write(s_tx_chan, s_tdm_write_buf, bytes_to_write, &bytes_written, timeout_ms);
        if (err != ESP_OK) {
            return err;
        }
        if (bytes_written != bytes_to_write) {
            return ESP_ERR_TIMEOUT;
        }

        frame_offset += frames_this_write;
    }
    return ESP_OK;
#else
    size_t bytes_written = 0;
    const size_t bytes_to_write = frame_count * 2U * sizeof(int16_t);
    esp_err_t err = i2s_channel_write(s_tx_chan, stereo_pcm, bytes_to_write, &bytes_written, timeout_ms);
    if (err != ESP_OK) {
        return err;
    }
    if (bytes_written != bytes_to_write) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
#endif
}

esp_err_t sc_audio_i2s_set_tx_enabled(bool enable)
{
    ESP_RETURN_ON_FALSE(s_tx_chan != NULL, ESP_ERR_INVALID_STATE, TAG, "tx channel not initialized");

    if (enable) {
        if (s_tx_enabled) {
            return ESP_OK;
        }
        ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx_chan), TAG, "i2s_channel_enable failed");
        s_tx_enabled = true;
        return ESP_OK;
    }

    if (!s_tx_enabled) {
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(i2s_channel_disable(s_tx_chan), TAG, "i2s_channel_disable failed");
    s_tx_enabled = false;
    return ESP_OK;
}

esp_err_t sc_audio_i2s_read(int16_t *stereo_pcm, size_t frame_count, size_t *frames_read, uint32_t timeout_ms)
{
#if (SOC_I2S_NUM <= 1)
    (void)stereo_pcm;
    (void)frame_count;
    (void)frames_read;
    (void)timeout_ms;
    return ESP_ERR_NOT_SUPPORTED;
#else
    size_t bytes_read = 0;
    const size_t bytes_to_read = frame_count * 2U * sizeof(int16_t);

    esp_err_t err = i2s_channel_read(s_rx_chan, stereo_pcm, bytes_to_read, &bytes_read, timeout_ms);
    if (err != ESP_OK) {
        return err;
    }
    if (frames_read != NULL) {
        *frames_read = bytes_read / (2U * sizeof(int16_t));
    }
    return ESP_OK;
#endif
}
