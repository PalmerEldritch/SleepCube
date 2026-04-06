#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SC_AUDIO_HPF_MAX_STAGES 4

typedef struct {
    float alpha;
    float prev_input;
    float prev_output;
} sc_hpf_stage_state_t;

typedef struct {
    sc_hpf_stage_state_t stages[SC_AUDIO_HPF_MAX_STAGES];
} sc_hpf_chain_t;

typedef struct {
    float alpha;
    float prev_output;
} sc_lpf_state_t;

float sc_audio_hpf_alpha(uint32_t sample_rate_hz, uint16_t cutoff_hz);
float sc_audio_lpf_alpha(uint32_t sample_rate_hz, uint16_t cutoff_hz);
void sc_audio_hpf_reset(sc_hpf_chain_t *chain);
int16_t sc_audio_hpf_process(sc_hpf_chain_t *chain, int16_t sample, float alpha, uint8_t stage_count);
bool sc_audio_hpf_enabled(uint16_t cutoff_hz, uint8_t stage_count);
void sc_audio_lpf_reset(sc_lpf_state_t *state);
int16_t sc_audio_lpf_process(sc_lpf_state_t *state, int16_t sample, float alpha);
