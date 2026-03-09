#include "audio_mp3.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "audio_i2s.h"
#include "mp3dec.h"

static const char *TAG = "sc_audio_mp3";

#define SC_MP3_INBUF_SIZE            (4096)
#define SC_MP3_REFILL_THRESHOLD      (1024)
#define SC_MP3_MAX_OUTPUT_SAMPLES    (1152 * 2)

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

esp_err_t sc_audio_mp3_play_file(const char *path, const volatile bool *play_enabled, uint8_t volume_percent)
{
    esp_err_t ret = ESP_OK;
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    HMP3Decoder decoder = MP3InitDecoder();
    if (decoder == NULL) {
        fclose(fp);
        ESP_LOGE(TAG, "MP3InitDecoder failed");
        return ESP_FAIL;
    }

    uint8_t *inbuf = (uint8_t *)malloc(SC_MP3_INBUF_SIZE);
    short *pcm_buf = (short *)malloc(SC_MP3_MAX_OUTPUT_SAMPLES * sizeof(short));
    int16_t *stereo_mono_buf = (int16_t *)malloc((SC_MP3_MAX_OUTPUT_SAMPLES * 2) * sizeof(int16_t));
    if ((inbuf == NULL) || (pcm_buf == NULL) || (stereo_mono_buf == NULL)) {
        free(inbuf);
        free(pcm_buf);
        free(stereo_mono_buf);
        MP3FreeDecoder(decoder);
        fclose(fp);
        ESP_LOGE(TAG, "failed to allocate decode buffers");
        return ESP_ERR_NO_MEM;
    }

    int bytes_left = 0;
    uint8_t *read_ptr = inbuf;
    bool warned_rate_mismatch = false;

    while (1) {
        if ((play_enabled != NULL) && !(*play_enabled)) {
            break;
        }

        if (bytes_left < SC_MP3_REFILL_THRESHOLD) {
            if (bytes_left > 0 && read_ptr != inbuf) {
                memmove(inbuf, read_ptr, (size_t)bytes_left);
            }
            read_ptr = inbuf;

            size_t n = fread(inbuf + bytes_left, 1, (size_t)(SC_MP3_INBUF_SIZE - bytes_left), fp);
            bytes_left += (int)n;

            if (n == 0 && bytes_left == 0) {
                break;
            }
        }

        int sync_offset = MP3FindSyncWord(read_ptr, bytes_left);
        if (sync_offset < 0) {
            if (bytes_left > 4) {
                read_ptr += (bytes_left - 4);
                bytes_left = 4;
            }
            continue;
        }

        read_ptr += sync_offset;
        bytes_left -= sync_offset;

        unsigned char *decode_ptr = read_ptr;
        int decode_bytes_left = bytes_left;
        int dec_err = MP3Decode(decoder, &decode_ptr, &decode_bytes_left, pcm_buf, 0);
        int consumed = bytes_left - decode_bytes_left;
        read_ptr += consumed;
        bytes_left = decode_bytes_left;

        if (dec_err != ERR_MP3_NONE) {
            continue;
        }

        MP3FrameInfo frame_info;
        MP3GetLastFrameInfo(decoder, &frame_info);

        if (!warned_rate_mismatch && frame_info.samprate != 44100) {
            warned_rate_mismatch = true;
            ESP_LOGW(TAG, "mp3 sample rate is %d Hz, playback I2S is fixed at 44100 Hz", frame_info.samprate);
        }

        if (frame_info.outputSamps <= 0) {
            continue;
        }

        if (frame_info.nChans == 2) {
            size_t frames = (size_t)frame_info.outputSamps / 2U;
            size_t sample_count = frames * 2U;
            for (size_t i = 0; i < sample_count; i++) {
                pcm_buf[i] = (short)sc_scale_sample((int16_t)pcm_buf[i], volume_percent);
            }
            ret = sc_audio_i2s_write((const int16_t *)pcm_buf, frames, 1000);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "i2s write failed");
                break;
            }
        } else if (frame_info.nChans == 1) {
            size_t mono_samples = (size_t)frame_info.outputSamps;
            for (size_t i = 0; i < mono_samples; i++) {
                const int16_t s = sc_scale_sample((int16_t)pcm_buf[i], volume_percent);
                stereo_mono_buf[(i * 2) + 0] = s;
                stereo_mono_buf[(i * 2) + 1] = s;
            }
            ret = sc_audio_i2s_write(stereo_mono_buf, mono_samples, 1000);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "i2s write failed");
                break;
            }
        }
    }

    free(inbuf);
    free(pcm_buf);
    free(stereo_mono_buf);
    MP3FreeDecoder(decoder);
    fclose(fp);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "finished playback: %s", path);
    }
    return ret;
}
