#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sc_audio_protocol.h"

typedef struct {
    uint32_t phase;
    uint32_t phase_step;
    sc_audio_tone_note_t note;
} sc_tone_generator_t;

void sc_tone_generator_init(sc_tone_generator_t *generator,
                            sc_audio_tone_note_t note);
void sc_tone_generator_set_note(sc_tone_generator_t *generator,
                                sc_audio_tone_note_t note);
void sc_tone_generator_fill(sc_tone_generator_t *generator,
                            int16_t *samples,
                            size_t frame_count);
uint32_t sc_tone_generator_frequency_millihz(sc_audio_tone_note_t note);
