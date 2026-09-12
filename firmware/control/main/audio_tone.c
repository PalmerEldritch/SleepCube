#include "audio_tone.h"

#include <math.h>

static const float SC_TONE_TWO_PI = 6.28318530718f;

void sc_tone_set_frequency(sc_tone_state_t *state, uint32_t sample_rate_hz, float frequency_hz)
{
    state->phase_step = (SC_TONE_TWO_PI * frequency_hz) / (float)sample_rate_hz;
}

void sc_tone_init(sc_tone_state_t *state, uint32_t sample_rate_hz, float frequency_hz, float amplitude)
{
    state->phase = 0.0f;
    state->amplitude = amplitude;
    sc_tone_set_frequency(state, sample_rate_hz, frequency_hz);
}

int16_t sc_tone_next_sample(sc_tone_state_t *state)
{
    const float s = sinf(state->phase) * state->amplitude;
    const int16_t pcm = (int16_t)(s * 32767.0f);
    state->phase += state->phase_step;
    if (state->phase >= SC_TONE_TWO_PI) {
        state->phase -= SC_TONE_TWO_PI;
    }
    return pcm;
}
