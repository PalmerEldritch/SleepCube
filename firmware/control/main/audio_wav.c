#include "audio_wav.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "audio_player.h"
#include "audio_i2s.h"
#include "lvgl_display_if.h"

static const char *TAG = "sc_audio_wav";

#define SC_WAV_READ_BYTES           (4096U)
#define SC_WAV_READ_LOCK_TIMEOUT_MS (1000U)
#define SC_WAV_PCM_FMT              (1U)

typedef struct {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate_hz;
    uint16_t bits_per_sample;
    uint32_t data_size_bytes;
} sc_wav_header_t;

static uint16_t sc_wav_le16(const uint8_t *buf)
{
    return (uint16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
}

static uint32_t sc_wav_le32(const uint8_t *buf)
{
    return (uint32_t)buf[0] |
           ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}

static int16_t sc_scale_sample(int16_t sample, uint8_t volume_percent)
{
    int32_t scaled = ((int32_t)sample * (int32_t)volume_percent) / 100;
    if (scaled > 32767) {
        scaled = 32767;
    }
    if (scaled < -32768) {
        scaled = -32768;
    }
    return (int16_t)scaled;
}

static esp_err_t sc_wav_parse_header(FILE *fp, sc_wav_header_t *header)
{
    uint8_t chunk[12];
    if ((fp == NULL) || (header == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (fread(chunk, 1, sizeof(chunk), fp) != sizeof(chunk)) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    if ((memcmp(&chunk[0], "RIFF", 4) != 0) || (memcmp(&chunk[8], "WAVE", 4) != 0)) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    bool found_fmt = false;
    bool found_data = false;
    memset(header, 0, sizeof(*header));

    while (!found_data) {
        uint8_t subhdr[8];
        if (fread(subhdr, 1, sizeof(subhdr), fp) != sizeof(subhdr)) {
            return ESP_ERR_INVALID_RESPONSE;
        }

        uint32_t chunk_size = sc_wav_le32(&subhdr[4]);
        if (memcmp(&subhdr[0], "fmt ", 4) == 0) {
            uint8_t fmt_buf[16];
            if (chunk_size < sizeof(fmt_buf)) {
                return ESP_ERR_NOT_SUPPORTED;
            }
            if (fread(fmt_buf, 1, sizeof(fmt_buf), fp) != sizeof(fmt_buf)) {
                return ESP_ERR_INVALID_RESPONSE;
            }
            if (chunk_size > sizeof(fmt_buf)) {
                if (fseek(fp, (long)(chunk_size - sizeof(fmt_buf)), SEEK_CUR) != 0) {
                    return ESP_ERR_INVALID_RESPONSE;
                }
            }

            header->audio_format = sc_wav_le16(&fmt_buf[0]);
            header->num_channels = sc_wav_le16(&fmt_buf[2]);
            header->sample_rate_hz = sc_wav_le32(&fmt_buf[4]);
            header->bits_per_sample = sc_wav_le16(&fmt_buf[14]);
            found_fmt = true;
        } else if (memcmp(&subhdr[0], "data", 4) == 0) {
            header->data_size_bytes = chunk_size;
            found_data = true;
            break;
        } else {
            if (fseek(fp, (long)chunk_size, SEEK_CUR) != 0) {
                return ESP_ERR_INVALID_RESPONSE;
            }
        }

        if ((chunk_size & 1U) != 0U) {
            if (fseek(fp, 1L, SEEK_CUR) != 0) {
                return ESP_ERR_INVALID_RESPONSE;
            }
        }
    }

    if (!found_fmt || !found_data) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if ((header->audio_format != SC_WAV_PCM_FMT) ||
        ((header->num_channels != 1U) && (header->num_channels != 2U)) ||
        (header->bits_per_sample != 16U)) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ESP_OK;
}

esp_err_t sc_audio_wav_play_file(const char *path, const volatile bool *play_enabled)
{
    esp_err_t ret = ESP_OK;
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    sc_wav_header_t header;
    ret = sc_wav_parse_header(fp, &header);
    if (ret != ESP_OK) {
        fclose(fp);
        ESP_LOGE(TAG, "unsupported or invalid wav: %s", path);
        return ret;
    }

    if (header.sample_rate_hz != 44100U) {
        ESP_LOGW(TAG, "wav sample rate is %lu Hz, playback I2S is fixed at 44100 Hz",
                 (unsigned long)header.sample_rate_hz);
    }

    uint8_t *read_buf = (uint8_t *)malloc(SC_WAV_READ_BYTES);
    int16_t *mono_to_stereo = (int16_t *)malloc((SC_WAV_READ_BYTES / 2U) * 2U * sizeof(int16_t));
    if ((read_buf == NULL) || (mono_to_stereo == NULL)) {
        free(read_buf);
        free(mono_to_stereo);
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    uint32_t bytes_remaining = header.data_size_bytes;
    while (bytes_remaining > 0U) {
        if ((play_enabled != NULL) && !(*play_enabled)) {
            break;
        }
        if (sc_audio_player_get_source_mode() != SC_AUDIO_SOURCE_MODE_MP3) {
            break;
        }

        size_t to_read = (bytes_remaining < SC_WAV_READ_BYTES) ? (size_t)bytes_remaining : SC_WAV_READ_BYTES;

        bool locked = sc_lvgl_display_bus_lock(SC_WAV_READ_LOCK_TIMEOUT_MS);
        if (!locked) {
            ret = ESP_ERR_TIMEOUT;
            ESP_LOGE(TAG, "lvgl bus lock timeout before WAV read");
            break;
        }
        size_t got = fread(read_buf, 1, to_read, fp);
        sc_lvgl_display_bus_unlock();

        if (got == 0U) {
            break;
        }
        bytes_remaining -= (uint32_t)got;

        const uint8_t volume_percent = sc_audio_player_get_effective_volume_percent();
        if (header.num_channels == 2U) {
            size_t sample_count = got / sizeof(int16_t);
            int16_t *pcm = (int16_t *)read_buf;
            for (size_t i = 0; i < sample_count; i++) {
                pcm[i] = sc_scale_sample(pcm[i], volume_percent);
            }
            ret = sc_audio_i2s_write(pcm, sample_count / 2U, 1000U);
        } else {
            size_t mono_samples = got / sizeof(int16_t);
            int16_t *pcm = (int16_t *)read_buf;
            for (size_t i = 0; i < mono_samples; i++) {
                int16_t s = sc_scale_sample(pcm[i], volume_percent);
                mono_to_stereo[(i * 2U) + 0U] = s;
                mono_to_stereo[(i * 2U) + 1U] = s;
            }
            ret = sc_audio_i2s_write(mono_to_stereo, mono_samples, 1000U);
        }

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "i2s write failed");
            break;
        }
    }

    free(read_buf);
    free(mono_to_stereo);
    fclose(fp);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "finished playback: %s", path);
    }
    return ret;
}
