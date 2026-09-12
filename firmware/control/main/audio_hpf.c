#include "audio_hpf.h"

#include <stddef.h>

#define SC_AUDIO_TWO_PI  (6.28318530717958647692f)

float sc_audio_hpf_alpha(uint32_t sample_rate_hz, uint16_t cutoff_hz)
{
    if ((sample_rate_hz == 0U) || (cutoff_hz == 0U)) {
        return 0.0f;
    }

    const float dt = 1.0f / (float)sample_rate_hz;
    const float rc = 1.0f / (SC_AUDIO_TWO_PI * (float)cutoff_hz);
    return rc / (rc + dt);
}

uint16_t sc_audio_hpf_alpha_q15(uint32_t sample_rate_hz, uint16_t cutoff_hz)
{
    const float alpha = sc_audio_hpf_alpha(sample_rate_hz, cutoff_hz);
    if (alpha <= 0.0f) {
        return 0U;
    }
    if (alpha >= 1.0f) {
        return 32767U;
    }
    return (uint16_t)(alpha * 32767.0f);
}

float sc_audio_lpf_alpha(uint32_t sample_rate_hz, uint16_t cutoff_hz)
{
    if ((sample_rate_hz == 0U) || (cutoff_hz == 0U)) {
        return 0.0f;
    }

    const float dt = 1.0f / (float)sample_rate_hz;
    const float rc = 1.0f / (SC_AUDIO_TWO_PI * (float)cutoff_hz);
    return dt / (rc + dt);
}

uint16_t sc_audio_lpf_alpha_q15(uint32_t sample_rate_hz, uint16_t cutoff_hz)
{
    const float alpha = sc_audio_lpf_alpha(sample_rate_hz, cutoff_hz);
    if (alpha <= 0.0f) {
        return 0U;
    }
    if (alpha >= 1.0f) {
        return 32767U;
    }
    return (uint16_t)(alpha * 32767.0f);
}

void sc_audio_hpf_reset(sc_hpf_chain_t *chain)
{
    if (chain == NULL) {
        return;
    }

    for (size_t i = 0; i < SC_AUDIO_HPF_MAX_STAGES; i++) {
        chain->stages[i].prev_input = 0;
        chain->stages[i].prev_output = 0;
    }
}

void sc_audio_lpf_reset(sc_lpf_state_t *state)
{
    if (state == NULL) {
        return;
    }

    state->prev_output = 0.0f;
}

void sc_audio_lpf_chain_reset(sc_lpf_chain_t *chain)
{
    if (chain == NULL) {
        return;
    }

    for (size_t i = 0; i < SC_AUDIO_LPF_MAX_STAGES; i++) {
        sc_audio_lpf_reset(&chain->stages[i]);
    }
}

static int16_t sc_audio_hpf_process_stage_q15(sc_hpf_stage_state_t *state, int16_t sample, uint16_t alpha_q15)
{
    const int32_t input = (int32_t)sample;
    const int32_t delta = state->prev_output + input - state->prev_input;
    const int32_t output = (int32_t)(((int64_t)delta * (int64_t)alpha_q15 + 16384LL) >> 15);
    state->prev_input = input;
    state->prev_output = output;

    if (output > 32767) {
        return 32767;
    }
    if (output < -32768) {
        return -32768;
    }
    return (int16_t)output;
}

int16_t sc_audio_lpf_process(sc_lpf_state_t *state, int16_t sample, float alpha)
{
    uint16_t alpha_q15 = 0U;
    if (alpha > 0.0f) {
        if (alpha >= 1.0f) {
            alpha_q15 = 32767U;
        } else {
            alpha_q15 = (uint16_t)(alpha * 32767.0f);
        }
    }
    return sc_audio_lpf_process_q15(state, sample, alpha_q15);
}

int16_t sc_audio_lpf_process_q15(sc_lpf_state_t *state, int16_t sample, uint16_t alpha_q15)
{
    if ((state == NULL) || (alpha_q15 == 0U)) {
        return sample;
    }

    const float input = (float)sample;
    const float alpha = (float)alpha_q15 / 32767.0f;
    const float output = state->prev_output + (alpha * (input - state->prev_output));
    state->prev_output = output;

    if (output > 32767.0f) {
        return 32767;
    }
    if (output < -32768.0f) {
        return -32768;
    }
    return (int16_t)output;
}

int16_t sc_audio_lpf_chain_process(sc_lpf_chain_t *chain, int16_t sample, float alpha, uint8_t stage_count)
{
    uint16_t alpha_q15 = 0U;
    if (alpha > 0.0f) {
        if (alpha >= 1.0f) {
            alpha_q15 = 32767U;
        } else {
            alpha_q15 = (uint16_t)(alpha * 32767.0f);
        }
    }
    return sc_audio_lpf_chain_process_q15(chain, sample, alpha_q15, stage_count);
}

int16_t sc_audio_lpf_chain_process_q15(sc_lpf_chain_t *chain, int16_t sample, uint16_t alpha_q15, uint8_t stage_count)
{
    if ((chain == NULL) || (alpha_q15 == 0U) || (stage_count == 0U)) {
        return sample;
    }

    if (stage_count > SC_AUDIO_LPF_MAX_STAGES) {
        stage_count = SC_AUDIO_LPF_MAX_STAGES;
    }

    int16_t filtered = sample;
    for (uint8_t i = 0; i < stage_count; i++) {
        filtered = sc_audio_lpf_process_q15(&chain->stages[i], filtered, alpha_q15);
    }
    return filtered;
}

int16_t sc_audio_hpf_process(sc_hpf_chain_t *chain, int16_t sample, float alpha, uint8_t stage_count)
{
    uint16_t alpha_q15 = 0U;
    if (alpha > 0.0f) {
        if (alpha >= 1.0f) {
            alpha_q15 = 32767U;
        } else {
            alpha_q15 = (uint16_t)(alpha * 32767.0f);
        }
    }
    return sc_audio_hpf_process_q15(chain, sample, alpha_q15, stage_count);
}

int16_t sc_audio_hpf_process_q15(sc_hpf_chain_t *chain, int16_t sample, uint16_t alpha_q15, uint8_t stage_count)
{
    if ((chain == NULL) || (alpha_q15 == 0U) || (stage_count == 0U)) {
        return sample;
    }

    if (stage_count > SC_AUDIO_HPF_MAX_STAGES) {
        stage_count = SC_AUDIO_HPF_MAX_STAGES;
    }

    int16_t filtered = sample;
    for (uint8_t i = 0; i < stage_count; i++) {
        filtered = sc_audio_hpf_process_stage_q15(&chain->stages[i], filtered, alpha_q15);
    }
    return filtered;
}

bool sc_audio_hpf_enabled(uint16_t cutoff_hz, uint8_t stage_count)
{
    return (cutoff_hz > 0U) && (stage_count > 0U);
}

bool sc_audio_lpf_enabled(uint16_t cutoff_hz, uint8_t stage_count)
{
    return (cutoff_hz > 0U) && (stage_count > 0U);
}
