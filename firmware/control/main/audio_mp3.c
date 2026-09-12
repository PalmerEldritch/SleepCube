#include "audio_mp3.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_check.h"
#include "esp_log.h"
#include "audio_player.h"
#include "audio_hpf.h"
#include "audio_i2s.h"
#include "lvgl_display_if.h"
#include "mp3dec.h"

static const char *TAG = "sc_audio_mp3";

#define SC_MP3_INBUF_SIZE            (4096)
#define SC_MP3_REFILL_THRESHOLD      (1024)
#define SC_MP3_MAX_OUTPUT_SAMPLES    (1152 * 2)
#define SC_SD_READ_LOCK_TIMEOUT_MS   (1000U)

static int32_t sc_pre_gain_scale_q15(uint8_t pre_gain_db)
{
    if (pre_gain_db == 0U) {
        return 32768;
    }

    const float linear = powf(10.0f, -((float)pre_gain_db) / 20.0f);
    int32_t scale = (int32_t)(linear * 32768.0f);
    if (scale < 0) {
        scale = 0;
    }
    if (scale > 32768) {
        scale = 32768;
    }
    return scale;
}

static int16_t sc_apply_pre_gain_scale(int16_t sample, int32_t scale_q15)
{
    int32_t scaled = ((int32_t)sample * scale_q15) / 32768;
    if (scaled > 32767) {
        scaled = 32767;
    }
    if (scaled < -32768) {
        scaled = -32768;
    }
    return (int16_t)scaled;
}

static int16_t sc_apply_soft_limiter(int16_t sample, bool enabled, uint8_t threshold_pct)
{
    if (!enabled) {
        return sample;
    }

    if (threshold_pct < 50U) {
        threshold_pct = 50U;
    }
    if (threshold_pct > 99U) {
        threshold_pct = 99U;
    }

    const float threshold = (float)threshold_pct / 100.0f;
    const float sign = (sample < 0) ? -1.0f : 1.0f;
    float magnitude = (float)sample / 32767.0f;
    if (magnitude < 0.0f) {
        magnitude = -magnitude;
    }

    if (magnitude <= threshold) {
        return sample;
    }

    float over = (magnitude - threshold) / (1.0f - threshold);
    if (over < 0.0f) {
        over = 0.0f;
    } else if (over > 1.0f) {
        over = 1.0f;
    }

    const float shaped = threshold + ((1.0f - threshold) * (over * over));
    int32_t limited = (int32_t)(sign * shaped * 32767.0f);
    if (limited > 32767) {
        limited = 32767;
    }
    if (limited < -32768) {
        limited = -32768;
    }
    return (int16_t)limited;
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

static int16_t sc_apply_mp3_eq_sample(int16_t sample,
                                      sc_audio_mp3_eq_mode_t eq_mode,
                                      sc_lpf_state_t *lpf_state,
                                      float lpf_alpha,
                                      uint8_t depth_pct)
{
    if ((eq_mode == SC_AUDIO_MP3_EQ_OFF) || (lpf_state == NULL) || (lpf_alpha <= 0.0f)) {
        return sample;
    }

    if (depth_pct > 100U) {
        depth_pct = 100U;
    }

    const int16_t low = sc_audio_lpf_process(lpf_state, sample, lpf_alpha);
    if (eq_mode == SC_AUDIO_MP3_EQ_LPF) {
        return low;
    }

    int32_t high = (int32_t)sample - (int32_t)low;
    int32_t kept_high = (high * (int32_t)(100U - depth_pct)) / 100;
    int32_t out = (int32_t)low + kept_high;
    if (out > 32767) {
        out = 32767;
    }
    if (out < -32768) {
        out = -32768;
    }
    return (int16_t)out;
}

esp_err_t sc_audio_mp3_play_file(const char *path, const volatile bool *play_enabled)
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
    bool logged_hpf = false;
    sc_hpf_chain_t left_hpf = { 0 };
    sc_hpf_chain_t right_hpf = { 0 };
    sc_hpf_chain_t mono_hpf = { 0 };
    bool logged_lpf = false;
    sc_lpf_chain_t left_lpf_global = { 0 };
    sc_lpf_chain_t right_lpf_global = { 0 };
    sc_lpf_chain_t mono_lpf_global = { 0 };
    sc_lpf_state_t left_lpf = { 0 };
    sc_lpf_state_t right_lpf = { 0 };
    sc_lpf_state_t mono_lpf = { 0 };

    while (1) {
        if ((play_enabled != NULL) && !(*play_enabled)) {
            break;
        }
        if (sc_audio_player_get_source_mode() != SC_AUDIO_SOURCE_MODE_MP3) {
            break;
        }

        if (bytes_left < SC_MP3_REFILL_THRESHOLD) {
            if (bytes_left > 0 && read_ptr != inbuf) {
                memmove(inbuf, read_ptr, (size_t)bytes_left);
            }
            read_ptr = inbuf;

            bool locked = sc_lvgl_display_bus_lock(SC_SD_READ_LOCK_TIMEOUT_MS);
            if (!locked) {
                ret = ESP_ERR_TIMEOUT;
                ESP_LOGE(TAG, "lvgl bus lock timeout before SD read");
                break;
            }

            size_t n = fread(inbuf + bytes_left, 1, (size_t)(SC_MP3_INBUF_SIZE - bytes_left), fp);
            sc_lvgl_display_bus_unlock();
            bytes_left += (int)n;

            if (n == 0 && bytes_left == 0) {
                break;
            }
        }

        int sync_offset = MP3FindSyncWord(read_ptr, bytes_left);
        if (sync_offset < 0) {
            sc_audio_player_note_mp3_sync_miss();
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
            sc_audio_player_note_mp3_decode_error();
            continue;
        }

        MP3FrameInfo frame_info;
        MP3GetLastFrameInfo(decoder, &frame_info);

        if (!warned_rate_mismatch && frame_info.samprate != 44100) {
            warned_rate_mismatch = true;
            sc_audio_player_note_mp3_rate_mismatch();
            ESP_LOGW(TAG, "mp3 sample rate is %d Hz, playback I2S is fixed at 44100 Hz", frame_info.samprate);
        }

        if (frame_info.outputSamps <= 0) {
            continue;
        }

        const uint16_t hpf_cutoff_hz = sc_audio_player_get_hpf_cutoff_hz();
        const uint8_t hpf_stages = sc_audio_player_get_hpf_stages();
        const float hpf_alpha = sc_audio_hpf_alpha((uint32_t)frame_info.samprate, hpf_cutoff_hz);
        const uint16_t hpf_alpha_q15 = sc_audio_hpf_alpha_q15((uint32_t)frame_info.samprate, hpf_cutoff_hz);
        const uint16_t lpf_cutoff_hz = sc_audio_player_get_lpf_cutoff_hz();
        const uint8_t lpf_stages = sc_audio_player_get_lpf_stages();
        const float lpf_alpha_global = sc_audio_lpf_alpha((uint32_t)frame_info.samprate, lpf_cutoff_hz);
        const uint16_t lpf_alpha_q15 = sc_audio_lpf_alpha_q15((uint32_t)frame_info.samprate, lpf_cutoff_hz);
        const sc_audio_mp3_eq_mode_t eq_mode = sc_audio_player_get_mp3_eq_mode();
        const uint16_t eq_cutoff_hz = sc_audio_player_get_mp3_eq_cutoff_hz();
        const uint8_t eq_depth_pct = sc_audio_player_get_mp3_eq_depth_pct();
        const float lpf_alpha = sc_audio_lpf_alpha((uint32_t)frame_info.samprate, eq_cutoff_hz);

        if (!logged_hpf) {
            if (sc_audio_hpf_enabled(hpf_cutoff_hz, hpf_stages)) {
                ESP_LOGI(TAG, "software HPF enabled: cutoff=%u Hz stages=%u alpha=%.5f",
                         (unsigned)hpf_cutoff_hz, (unsigned)hpf_stages, (double)hpf_alpha);
            } else {
                ESP_LOGI(TAG, "software HPF disabled");
            }
            logged_hpf = true;
        }
        if (!logged_lpf) {
            if (sc_audio_lpf_enabled(lpf_cutoff_hz, lpf_stages)) {
                ESP_LOGI(TAG, "software LPF enabled: cutoff=%u Hz stages=%u alpha=%.5f",
                         (unsigned)lpf_cutoff_hz, (unsigned)lpf_stages, (double)lpf_alpha_global);
            } else {
                ESP_LOGI(TAG, "software LPF disabled");
            }
            logged_lpf = true;
        }

        if (frame_info.nChans == 2) {
            size_t frames = (size_t)frame_info.outputSamps / 2U;
            size_t sample_count = frames * 2U;
            const uint8_t volume_percent = sc_audio_player_get_effective_volume_percent();
            const sc_audio_mp3_mix_mode_t mix_mode = sc_audio_player_get_mp3_mix_mode();
            const uint8_t pre_gain_db = sc_audio_player_get_mp3_pre_gain_db();
            const int32_t pre_gain_scale_q15 = sc_pre_gain_scale_q15(pre_gain_db);
            const bool limiter_enabled = sc_audio_player_get_mp3_limiter_enabled();
            const uint8_t limiter_threshold_pct = sc_audio_player_get_mp3_limiter_threshold_pct();
            for (size_t i = 0; i < sample_count; i += 2U) {
                int16_t left = (int16_t)pcm_buf[i];
                int16_t right = (int16_t)pcm_buf[i + 1U];
                int16_t out_left = left;
                int16_t out_right = right;

                switch (mix_mode) {
                    case SC_AUDIO_MP3_MIX_MONO: {
                        int32_t mixed = ((int32_t)left + (int32_t)right) / 2;
                        out_left = (int16_t)mixed;
                        out_right = (int16_t)mixed;
                        break;
                    }
                    case SC_AUDIO_MP3_MIX_LEFT:
                        out_left = left;
                        out_right = left;
                        break;
                    case SC_AUDIO_MP3_MIX_RIGHT:
                        out_left = right;
                        out_right = right;
                        break;
                    case SC_AUDIO_MP3_MIX_STEREO:
                    default:
                        break;
                }

                if (sc_audio_hpf_enabled(hpf_cutoff_hz, hpf_stages)) {
                    out_left = sc_audio_hpf_process_q15(&left_hpf, out_left, hpf_alpha_q15, hpf_stages);
                    out_right = sc_audio_hpf_process_q15(&right_hpf, out_right, hpf_alpha_q15, hpf_stages);
                }
                if (sc_audio_lpf_enabled(lpf_cutoff_hz, lpf_stages)) {
                    out_left = sc_audio_lpf_chain_process_q15(&left_lpf_global, out_left, lpf_alpha_q15, lpf_stages);
                    out_right = sc_audio_lpf_chain_process_q15(&right_lpf_global, out_right, lpf_alpha_q15, lpf_stages);
                }

                out_left = sc_apply_mp3_eq_sample(out_left, eq_mode, &left_lpf, lpf_alpha, eq_depth_pct);
                out_right = sc_apply_mp3_eq_sample(out_right, eq_mode, &right_lpf, lpf_alpha, eq_depth_pct);

                out_left = sc_apply_pre_gain_scale(out_left, pre_gain_scale_q15);
                out_right = sc_apply_pre_gain_scale(out_right, pre_gain_scale_q15);
                out_left = sc_scale_sample(out_left, volume_percent);
                out_right = sc_scale_sample(out_right, volume_percent);
                out_left = sc_apply_soft_limiter(out_left, limiter_enabled, limiter_threshold_pct);
                out_right = sc_apply_soft_limiter(out_right, limiter_enabled, limiter_threshold_pct);

                pcm_buf[i] = (short)out_left;
                pcm_buf[i + 1U] = (short)out_right;
            }
            ret = sc_audio_i2s_write((const int16_t *)pcm_buf, frames, 1000);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "i2s write failed");
                break;
            }
        } else if (frame_info.nChans == 1) {
            size_t mono_samples = (size_t)frame_info.outputSamps;
            const uint8_t volume_percent = sc_audio_player_get_effective_volume_percent();
            const uint8_t pre_gain_db = sc_audio_player_get_mp3_pre_gain_db();
            const int32_t pre_gain_scale_q15 = sc_pre_gain_scale_q15(pre_gain_db);
            const bool limiter_enabled = sc_audio_player_get_mp3_limiter_enabled();
            const uint8_t limiter_threshold_pct = sc_audio_player_get_mp3_limiter_threshold_pct();
            for (size_t i = 0; i < mono_samples; i++) {
                int16_t s = (int16_t)pcm_buf[i];
                if (sc_audio_hpf_enabled(hpf_cutoff_hz, hpf_stages)) {
                    s = sc_audio_hpf_process_q15(&mono_hpf, s, hpf_alpha_q15, hpf_stages);
                }
                if (sc_audio_lpf_enabled(lpf_cutoff_hz, lpf_stages)) {
                    s = sc_audio_lpf_chain_process_q15(&mono_lpf_global, s, lpf_alpha_q15, lpf_stages);
                }
                s = sc_apply_mp3_eq_sample(s, eq_mode, &mono_lpf, lpf_alpha, eq_depth_pct);
                s = sc_apply_pre_gain_scale(s, pre_gain_scale_q15);
                s = sc_scale_sample(s, volume_percent);
                s = sc_apply_soft_limiter(s, limiter_enabled, limiter_threshold_pct);
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
