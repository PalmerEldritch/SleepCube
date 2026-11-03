#include "audio_output.h"

#include <inttypes.h>
#include <stdbool.h>

#include "app_config.h"
#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "pcm_format.h"

static const char *TAG = "audio_output";

static bool format_is_valid(uint32_t sample_rate_hz,
                            uint8_t data_bits,
                            uint8_t slot_bits)
{
    return sample_rate_hz >= 8000U && sample_rate_hz <= 96000U &&
           (data_bits == 16U || data_bits == 24U) &&
           (slot_bits == data_bits || slot_bits == 32U);
}

static i2s_data_bit_width_t data_width(uint8_t bits)
{
    return bits == 24U ? I2S_DATA_BIT_WIDTH_24BIT
                       : I2S_DATA_BIT_WIDTH_16BIT;
}

esp_err_t sc_audio_output_reconfigure(sc_audio_output_t *output,
                                      uint32_t sample_rate_hz,
                                      uint8_t data_bits,
                                      uint8_t slot_bits)
{
    ESP_RETURN_ON_FALSE(output != NULL, ESP_ERR_INVALID_ARG, TAG, "output is NULL");
    ESP_RETURN_ON_FALSE(format_is_valid(sample_rate_hz, data_bits, slot_bits),
                        ESP_ERR_INVALID_ARG, TAG, "unsupported I2S format");

    if (output->tx_channel != NULL) {
        sc_audio_output_shutdown(output);
    }

    i2s_chan_config_t channel_config =
        I2S_CHANNEL_DEFAULT_CONFIG(SC_AUDIO_I2S_PORT, I2S_ROLE_MASTER);
    channel_config.dma_desc_num = SC_AUDIO_DMA_DESC_NUM;
    /* ESP-IDF requires 24-bit DMA frame counts to be divisible by three. */
    channel_config.dma_frame_num =
        data_bits == 24U ? SC_AUDIO_DMA_FRAME_NUM_24BIT
                         : SC_AUDIO_DMA_FRAME_NUM;
    channel_config.auto_clear = true;

    esp_err_t result = i2s_new_channel(&channel_config, &output->tx_channel, NULL);
    if (result != ESP_OK) {
        output->tx_channel = NULL;
        return result;
    }

    i2s_std_clk_config_t clock_config =
        I2S_STD_CLK_DEFAULT_CONFIG(sample_rate_hz);
    if (slot_bits == 24U) {
        clock_config.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    }
    i2s_std_slot_config_t slot_config = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        data_width(data_bits), I2S_SLOT_MODE_STEREO);
    slot_config.slot_bit_width = (i2s_slot_bit_width_t)slot_bits;
    slot_config.ws_width = slot_bits;

    const i2s_std_config_t standard_config = {
        .clk_cfg = clock_config,
        .slot_cfg = slot_config,
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
    output->sample_rate_hz = sample_rate_hz;
    output->data_bits = data_bits;
    output->slot_bits = slot_bits;
    ESP_LOGI(TAG,
             "I2S output enabled: rate=%" PRIu32 "Hz data=%ubit slot=%ubit "
             "stereo=duplicated-mono state=silent",
             sample_rate_hz, data_bits, slot_bits);
    return ESP_OK;
}

esp_err_t sc_audio_output_init(sc_audio_output_t *output)
{
    return sc_audio_output_reconfigure(output,
                                       SC_AUDIO_SAMPLE_RATE_HZ,
                                       SC_AUDIO_BITS_PER_SAMPLE,
                                       SC_AUDIO_BITS_PER_SAMPLE);
}

size_t sc_audio_output_bytes_per_frame(const sc_audio_output_t *output)
{
    if (output == NULL || (output->data_bits != 16U && output->data_bits != 24U)) {
        return 0U;
    }
    return SC_AUDIO_CHANNEL_COUNT * (output->data_bits / 8U);
}

esp_err_t sc_audio_output_write_mono_q31(sc_audio_output_t *output,
                                         const int32_t *mono_frames,
                                         size_t frame_count)
{
    ESP_RETURN_ON_FALSE(output != NULL && output->initialized &&
                            output->tx_channel != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "output is not initialized");
    ESP_RETURN_ON_FALSE(mono_frames != NULL || frame_count == 0,
                        ESP_ERR_INVALID_ARG, TAG, "frames is NULL");
    ESP_RETURN_ON_FALSE(frame_count <= SC_PLAYBACK_BLOCK_FRAMES,
                        ESP_ERR_INVALID_SIZE, TAG, "frame block is too large");

    if (frame_count == 0) {
        return ESP_OK;
    }

    uint8_t packed[SC_PLAYBACK_BLOCK_FRAMES * SC_AUDIO_CHANNEL_COUNT * 3U];
    const size_t requested_bytes =
        frame_count * sc_audio_output_bytes_per_frame(output);
    ESP_RETURN_ON_FALSE(
        sc_pcm_pack_stereo_q31(mono_frames, frame_count, output->data_bits,
                               packed, sizeof(packed)) == requested_bytes,
        ESP_ERR_INVALID_SIZE, TAG, "PCM packing failed");
    size_t written_bytes = 0;
    const esp_err_t result = i2s_channel_write(output->tx_channel,
                                               packed,
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
    output->sample_rate_hz = 0U;
    output->data_bits = 0U;
    output->slot_bits = 0U;
}

void sc_audio_output_fail_safe(sc_audio_output_t *output)
{
    sc_audio_output_shutdown(output);
    ESP_LOGE(TAG, "audio output disabled; reboot required before another attempt");
}
