#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SC_AUDIO_HPF_MAX_STAGES 4
#define SC_AUDIO_LPF_MAX_STAGES 4

typedef struct {
    int32_t prev_input;
    int32_t prev_output;
} sc_hpf_stage_state_t;

typedef struct {
    sc_hpf_stage_state_t stages[SC_AUDIO_HPF_MAX_STAGES];
} sc_hpf_chain_t;

typedef struct {
    float prev_output;
} sc_lpf_state_t;

typedef struct {
    sc_lpf_state_t stages[SC_AUDIO_LPF_MAX_STAGES];
} sc_lpf_chain_t;

float sc_audio_hpf_alpha(uint32_t sample_rate_hz, uint16_t cutoff_hz);
uint16_t sc_audio_hpf_alpha_q15(uint32_t sample_rate_hz, uint16_t cutoff_hz);
float sc_audio_lpf_alpha(uint32_t sample_rate_hz, uint16_t cutoff_hz);
uint16_t sc_audio_lpf_alpha_q15(uint32_t sample_rate_hz, uint16_t cutoff_hz);
void sc_audio_hpf_reset(sc_hpf_chain_t *chain);
int16_t sc_audio_hpf_process(sc_hpf_chain_t *chain, int16_t sample, float alpha, uint8_t stage_count);
int16_t sc_audio_hpf_process_q15(sc_hpf_chain_t *chain, int16_t sample, uint16_t alpha_q15, uint8_t stage_count);
bool sc_audio_hpf_enabled(uint16_t cutoff_hz, uint8_t stage_count);
void sc_audio_lpf_reset(sc_lpf_state_t *state);
int16_t sc_audio_lpf_process(sc_lpf_state_t *state, int16_t sample, float alpha);
int16_t sc_audio_lpf_process_q15(sc_lpf_state_t *state, int16_t sample, uint16_t alpha_q15);
void sc_audio_lpf_chain_reset(sc_lpf_chain_t *chain);
int16_t sc_audio_lpf_chain_process(sc_lpf_chain_t *chain, int16_t sample, float alpha, uint8_t stage_count);
int16_t sc_audio_lpf_chain_process_q15(sc_lpf_chain_t *chain, int16_t sample, uint16_t alpha_q15, uint8_t stage_count);
bool sc_audio_lpf_enabled(uint16_t cutoff_hz, uint8_t stage_count);
