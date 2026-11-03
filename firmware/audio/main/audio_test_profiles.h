#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "sc_audio_protocol.h"

typedef struct {
    sc_audio_source_t source;
    const char *path;
    uint32_t sample_rate_hz;
    uint8_t data_bits;
} sc_audio_test_profile_t;

/** Resolve a development source to its generated-tone or mono-WAV profile. */
bool sc_audio_test_profile_get(sc_audio_source_t source,
                               sc_audio_test_profile_t *profile);

