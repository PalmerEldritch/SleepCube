#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t format_tag;
    uint16_t channel_count;
    uint32_t sample_rate_hz;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t container_bits;
    uint16_t valid_bits;
    bool integer_pcm;
} sc_wav_format_t;

/** Parse classic PCM or WAVE_FORMAT_EXTENSIBLE integer-PCM fmt data. */
bool sc_wav_format_parse(const uint8_t *data,
                         size_t data_size,
                         sc_wav_format_t *format);
