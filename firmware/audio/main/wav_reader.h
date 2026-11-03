#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_err.h"

typedef struct {
    FILE *file;
    uint32_t sample_rate_hz;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t bytes_remaining;
    uint16_t channel_count;
    uint16_t bits_per_sample;
} sc_wav_reader_t;

/** Open and validate the canonical M2 PCM WAV content file. */
esp_err_t sc_wav_reader_open(sc_wav_reader_t *reader, const char *path);

/** Open a mono integer-PCM WAV and require the supplied debug profile. */
esp_err_t sc_wav_reader_open_expected(sc_wav_reader_t *reader,
                                      const char *path,
                                      uint32_t sample_rate_hz,
                                      uint16_t bits_per_sample);

/**
 * Read mono samples left-aligned in signed Q31 form. A zero frame count denotes
 * end of content. Both packed little-endian signed-16 and signed-24 PCM are
 * supported without discarding source bits.
 */
esp_err_t sc_wav_reader_read_q31(sc_wav_reader_t *reader,
                                 int32_t *frames,
                                 size_t capacity_frames,
                                 size_t *frames_read);

/** Seek back to the first content frame for continuous presentation. */
esp_err_t sc_wav_reader_rewind(sc_wav_reader_t *reader);

void sc_wav_reader_close(sc_wav_reader_t *reader);
