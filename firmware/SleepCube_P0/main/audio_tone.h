#pragma once

#include <stdint.h>

typedef struct {
    float phase;
    float phase_step;
    float amplitude;
} sc_tone_state_t;

void sc_tone_init(sc_tone_state_t *state, uint32_t sample_rate_hz, float frequency_hz, float amplitude);
int16_t sc_tone_next_sample(sc_tone_state_t *state);
