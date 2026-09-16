#include "audio_output.h"

#include <stdbool.h>

#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "audio_output";

esp_err_t sc_audio_output_init(sc_audio_output_t *output)
{
    ESP_RETURN_ON_FALSE(output != NULL, ESP_ERR_INVALID_ARG, TAG, "output is NULL");
    ESP_RETURN_ON_FALSE(!output->initialized && output->tx_channel == NULL,
                        ESP_ERR_INVALID_STATE, TAG, "output is already initialized");

    i2s_chan_config_t channel_config =
        I2S_CHANNEL_DEFAULT_CONFIG(SC_AUDIO_I2S_PORT, I2S_ROLE_MASTER);
    channel_config.dma_desc_num = SC_AUDIO_DMA_DESC_NUM;
    channel_config.dma_frame_num = SC_AUDIO_DMA_FRAME_NUM;
    channel_config.auto_clear = true;

    esp_err_t result = i2s_new_channel(&channel_config, &output->tx_channel, NULL);
    if (result != ESP_OK) {
        output->tx_channel = NULL;
        return result;
    }

    const i2s_std_config_t standard_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SC_AUDIO_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = SC_AUDIO_I2S_BCLK_GPIO,
            .ws = SC_AUDIO_I2S_LRCLK_GPIO,
            .dout = SC_AUDIO_I2S_DATA_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    result = i2s_channel_init_std_mode(output->tx_channel, &standard_config);
    if (result != ESP_OK) {
        sc_audio_output_shutdown(output);
        return result;
    }

    result = i2s_channel_enable(output->tx_channel);
    if (result != ESP_OK) {
        sc_audio_output_shutdown(output);
        return result;
    }

    output->initialized = true;
    ESP_LOGI(TAG, "I2S output enabled in auto-clear silent state");
    return ESP_OK;
}

esp_err_t sc_audio_output_write_frames(sc_audio_output_t *output,
                                       const int16_t *interleaved_frames,
                                       size_t frame_count)
{
    ESP_RETURN_ON_FALSE(output != NULL && output->initialized &&
                            output->tx_channel != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "output is not initialized");
    ESP_RETURN_ON_FALSE(interleaved_frames != NULL || frame_count == 0,
                        ESP_ERR_INVALID_ARG, TAG, "frames is NULL");

    if (frame_count == 0) {
        return ESP_OK;
    }

    const size_t requested_bytes = frame_count * SC_AUDIO_BYTES_PER_FRAME;
    size_t written_bytes = 0;
    const esp_err_t result = i2s_channel_write(output->tx_channel,
                                               interleaved_frames,
                                               requested_bytes,
                                               &written_bytes,
                                               pdMS_TO_TICKS(SC_AUDIO_WRITE_TIMEOUT_MS));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(result));
        return result;
    }
    if (written_bytes != requested_bytes) {
        ESP_LOGE(TAG, "partial I2S write: requested=%u written=%u",
                 (unsigned)requested_bytes, (unsigned)written_bytes);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

void sc_audio_output_shutdown(sc_audio_output_t *output)
{
    if (output == NULL) {
        return;
    }

    if (output->tx_channel != NULL) {
        if (output->initialized) {
            const esp_err_t disable_result = i2s_channel_disable(output->tx_channel);
            if (disable_result != ESP_OK && disable_result != ESP_ERR_INVALID_STATE) {
                ESP_LOGW(TAG, "I2S disable failed: %s", esp_err_to_name(disable_result));
            }
        }
        const esp_err_t delete_result = i2s_del_channel(output->tx_channel);
        if (delete_result != ESP_OK) {
            ESP_LOGW(TAG, "I2S channel delete failed: %s", esp_err_to_name(delete_result));
        }
    }

    output->tx_channel = NULL;
    output->initialized = false;
}

void sc_audio_output_fail_safe(sc_audio_output_t *output)
{
    sc_audio_output_shutdown(output);
    ESP_LOGE(TAG, "audio output disabled; reboot required before another attempt");
}
