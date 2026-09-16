#include "diagnostic_tone.h"

#include <stdint.h>

#include "board_config.h"

#define SC_TONE_BLOCK_FRAMES       256U
#define SC_TONE_SILENCE_MS         500U
#define SC_TONE_FADE_MS            250U
#define SC_TONE_STEADY_MS          2000U
#define SC_TONE_PEAK_GAIN_Q15      2068U /* Approximately -24 dBFS. */
#define SC_PHASE_INDEX_SHIFT       24U
#define SC_FRAMES_FOR_MS_CONST(ms) \
    (((uint64_t)SC_AUDIO_SAMPLE_RATE_HZ * (ms)) / 1000U)
#define SC_DIAGNOSTIC_TOTAL_FRAMES \
    ((3U * SC_FRAMES_FOR_MS_CONST(SC_TONE_SILENCE_MS)) + \
     (2U * ((2U * SC_FRAMES_FOR_MS_CONST(SC_TONE_FADE_MS)) + \
            SC_FRAMES_FOR_MS_CONST(SC_TONE_STEADY_MS))))

_Static_assert(SC_DIAGNOSTIC_TOTAL_FRAMES == 286650U,
               "Update M1 evidence when the diagnostic sequence changes");
_Static_assert((SC_DIAGNOSTIC_TOTAL_FRAMES * SC_AUDIO_BYTES_PER_FRAME) == 1146600U,
               "Update M1 byte-count evidence when the audio format changes");

static const int16_t SINE_LUT[256] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602,
    6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
    32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683,
    27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868,
    18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179,
    6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602,
    -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
    -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
    -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179,
    -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804,
};

typedef struct {
    uint32_t phase;
    uint32_t phase_step;
} tone_oscillator_t;

static uint32_t frames_for_ms(uint32_t milliseconds)
{
    return (uint32_t)(((uint64_t)SC_AUDIO_SAMPLE_RATE_HZ * milliseconds) / 1000U);
}

static esp_err_t write_silence(sc_audio_output_t *output,
                               uint32_t frame_count,
                               size_t *total_frames)
{
    int16_t buffer[SC_TONE_BLOCK_FRAMES * SC_AUDIO_CHANNEL_COUNT] = {0};

    while (frame_count > 0) {
        const size_t block_frames = frame_count > SC_TONE_BLOCK_FRAMES
                                        ? SC_TONE_BLOCK_FRAMES
                                        : frame_count;
        const esp_err_t result = sc_audio_output_write_frames(output, buffer, block_frames);
        if (result != ESP_OK) {
            return result;
        }
        *total_frames += block_frames;
        frame_count -= block_frames;
    }

    return ESP_OK;
}

static esp_err_t write_tone_segment(sc_audio_output_t *output,
                                    uint32_t frequency_hz,
                                    size_t *total_frames)
{
    int16_t buffer[SC_TONE_BLOCK_FRAMES * SC_AUDIO_CHANNEL_COUNT];
    tone_oscillator_t oscillator = {
        .phase = 0,
        .phase_step = (uint32_t)(((uint64_t)frequency_hz << 32) /
                                 SC_AUDIO_SAMPLE_RATE_HZ),
    };
    const uint32_t fade_frames = frames_for_ms(SC_TONE_FADE_MS);
    const uint32_t steady_frames = frames_for_ms(SC_TONE_STEADY_MS);
    const uint32_t segment_frames = (2U * fade_frames) + steady_frames;
    uint32_t frame_index = 0;

    while (frame_index < segment_frames) {
        const size_t block_frames = (segment_frames - frame_index) > SC_TONE_BLOCK_FRAMES
                                        ? SC_TONE_BLOCK_FRAMES
                                        : (segment_frames - frame_index);

        for (size_t i = 0; i < block_frames; ++i) {
            const uint32_t position = frame_index + i;
            uint32_t envelope_q15 = SC_TONE_PEAK_GAIN_Q15;
            if (position < fade_frames) {
                envelope_q15 = (uint32_t)(((uint64_t)SC_TONE_PEAK_GAIN_Q15 * position) /
                                          fade_frames);
            } else if (position >= (fade_frames + steady_frames)) {
                const uint32_t remaining = segment_frames - position - 1U;
                envelope_q15 = (uint32_t)(((uint64_t)SC_TONE_PEAK_GAIN_Q15 * remaining) /
                                          fade_frames);
            }

            const int32_t sample = SINE_LUT[oscillator.phase >> SC_PHASE_INDEX_SHIFT];
            const int16_t output_sample =
                (int16_t)((sample * (int32_t)envelope_q15) >> 15);
            buffer[(i * SC_AUDIO_CHANNEL_COUNT)] = output_sample;
            buffer[(i * SC_AUDIO_CHANNEL_COUNT) + 1U] = output_sample;
            oscillator.phase += oscillator.phase_step;
        }

        const esp_err_t result = sc_audio_output_write_frames(output, buffer, block_frames);
        if (result != ESP_OK) {
            return result;
        }
        *total_frames += block_frames;
        frame_index += block_frames;
    }

    return ESP_OK;
}

esp_err_t sc_diagnostic_tone_play(sc_audio_output_t *output,
                                  size_t *frames_written)
{
    if (output == NULL || frames_written == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *frames_written = 0;
    esp_err_t result = write_silence(output,
                                    frames_for_ms(SC_TONE_SILENCE_MS),
                                    frames_written);
    if (result != ESP_OK) {
        return result;
    }

    result = write_tone_segment(output, 440U, frames_written);
    if (result != ESP_OK) {
        return result;
    }
    result = write_silence(output,
                           frames_for_ms(SC_TONE_SILENCE_MS),
                           frames_written);
    if (result != ESP_OK) {
        return result;
    }

    result = write_tone_segment(output, 1000U, frames_written);
    if (result != ESP_OK) {
        return result;
    }
    return write_silence(output,
                         frames_for_ms(SC_TONE_SILENCE_MS),
                         frames_written);
}
