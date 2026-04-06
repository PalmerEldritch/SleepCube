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

float sc_audio_lpf_alpha(uint32_t sample_rate_hz, uint16_t cutoff_hz)
{
    if ((sample_rate_hz == 0U) || (cutoff_hz == 0U)) {
        return 0.0f;
    }

    const float dt = 1.0f / (float)sample_rate_hz;
    const float rc = 1.0f / (SC_AUDIO_TWO_PI * (float)cutoff_hz);
    return dt / (rc + dt);
}

void sc_audio_hpf_reset(sc_hpf_chain_t *chain)
{
    if (chain == NULL) {
        return;
    }

    for (size_t i = 0; i < SC_AUDIO_HPF_MAX_STAGES; i++) {
        chain->stages[i].alpha = 0.0f;
        chain->stages[i].prev_input = 0.0f;
        chain->stages[i].prev_output = 0.0f;
    }
}

void sc_audio_lpf_reset(sc_lpf_state_t *state)
{
    if (state == NULL) {
        return;
    }

    state->alpha = 0.0f;
    state->prev_output = 0.0f;
}

static int16_t sc_audio_hpf_process_stage(sc_hpf_stage_state_t *state, int16_t sample, float alpha)
{
    const float input = (float)sample;
    const float output = alpha * (state->prev_output + input - state->prev_input);
    state->alpha = alpha;
    state->prev_input = input;
    state->prev_output = output;

    if (output > 32767.0f) {
        return 32767;
    }
    if (output < -32768.0f) {
        return -32768;
    }
    return (int16_t)output;
}

int16_t sc_audio_lpf_process(sc_lpf_state_t *state, int16_t sample, float alpha)
{
    if ((state == NULL) || (alpha <= 0.0f)) {
        return sample;
    }

    const float input = (float)sample;
    const float output = state->prev_output + (alpha * (input - state->prev_output));
    state->alpha = alpha;
    state->prev_output = output;

    if (output > 32767.0f) {
        return 32767;
    }
    if (output < -32768.0f) {
        return -32768;
    }
    return (int16_t)output;
}

int16_t sc_audio_hpf_process(sc_hpf_chain_t *chain, int16_t sample, float alpha, uint8_t stage_count)
{
    if ((chain == NULL) || (alpha <= 0.0f) || (stage_count == 0U)) {
        return sample;
    }

    if (stage_count > SC_AUDIO_HPF_MAX_STAGES) {
        stage_count = SC_AUDIO_HPF_MAX_STAGES;
    }

    int16_t filtered = sample;
    for (uint8_t i = 0; i < stage_count; i++) {
        filtered = sc_audio_hpf_process_stage(&chain->stages[i], filtered, alpha);
    }
    return filtered;
}

bool sc_audio_hpf_enabled(uint16_t cutoff_hz, uint8_t stage_count)
{
    return (cutoff_hz > 0U) && (stage_count > 0U);
}
