#pragma once

#include <stdint.h>

/**
 * @brief State container for generated sine tone.
 */
typedef struct {
    float phase;
    float phase_step;
    float amplitude;
} sc_tone_state_t;

/**
 * @brief Initialize sine tone generation state.
 *
 * @param state Pointer to tone state.
 * @param sample_rate_hz Sample rate in Hz.
 * @param frequency_hz Sine frequency in Hz.
 * @param amplitude Linear amplitude in range 0.0 to 1.0.
 *
 * @note Kept for standalone tone tests.
 */
void sc_tone_init(sc_tone_state_t *state, uint32_t sample_rate_hz, float frequency_hz, float amplitude);

/**
 * @brief Update tone frequency while preserving oscillator phase.
 *
 * @param state Pointer to tone state.
 * @param sample_rate_hz Sample rate in Hz.
 * @param frequency_hz New sine frequency in Hz.
 */
void sc_tone_set_frequency(sc_tone_state_t *state, uint32_t sample_rate_hz, float frequency_hz);

/**
 * @brief Generate next 16-bit PCM sample from tone state.
 *
 * @param state Pointer to tone state.
 * @return Signed 16-bit PCM sample.
 */
int16_t sc_tone_next_sample(sc_tone_state_t *state);
