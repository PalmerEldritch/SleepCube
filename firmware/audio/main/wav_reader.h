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

/** Read mono signed-16 frames. A zero frame count denotes end of content. */
esp_err_t sc_wav_reader_read(sc_wav_reader_t *reader,
                             int16_t *frames,
                             size_t capacity_frames,
                             size_t *frames_read);

/** Seek back to the first content frame for continuous presentation. */
esp_err_t sc_wav_reader_rewind(sc_wav_reader_t *reader);

void sc_wav_reader_close(sc_wav_reader_t *reader);
