#include "audio_i2s.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s_std.h"
#include "esp_check.h"
#include "board_pins.h"

static i2s_chan_handle_t s_tx_chan;
static i2s_chan_handle_t s_rx_chan;

esp_err_t sc_audio_i2s_init(uint32_t sample_rate_hz)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &s_tx_chan, NULL), "sc_audio_i2s", "i2s_new_channel failed");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate_hz),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
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

    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(s_tx_chan, &std_cfg), "sc_audio_i2s", "i2s_channel_init_std_mode failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(s_tx_chan), "sc_audio_i2s", "i2s_channel_enable failed");
    return ESP_OK;
}

esp_err_t sc_audio_i2s_init_rx(uint32_t sample_rate_hz)
{
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
}

esp_err_t sc_audio_i2s_write(const int16_t *stereo_pcm, size_t frame_count, uint32_t timeout_ms)
{
    size_t bytes_written = 0;
    const size_t bytes_to_write = frame_count * 2U * sizeof(int16_t);
    esp_err_t err = i2s_channel_write(s_tx_chan, stereo_pcm, bytes_to_write, &bytes_written, pdMS_TO_TICKS(timeout_ms));
    if (err != ESP_OK) {
        return err;
    }
    if (bytes_written != bytes_to_write) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t sc_audio_i2s_read(int16_t *stereo_pcm, size_t frame_count, size_t *frames_read, uint32_t timeout_ms)
{
    size_t bytes_read = 0;
    const size_t bytes_to_read = frame_count * 2U * sizeof(int16_t);

    esp_err_t err = i2s_channel_read(s_rx_chan, stereo_pcm, bytes_to_read, &bytes_read, pdMS_TO_TICKS(timeout_ms));
    if (err != ESP_OK) {
        return err;
    }
    if (frames_read != NULL) {
        *frames_read = bytes_read / (2U * sizeof(int16_t));
    }
    return ESP_OK;
}
