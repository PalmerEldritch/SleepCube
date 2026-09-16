#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2s_std.h"
#include "esp_err.h"

typedef struct {
    i2s_chan_handle_t tx_channel;
    bool initialized;
} sc_audio_output_t;

/** Initialize and enable the AtomS3 Lite I2S output in a silent state. */
esp_err_t sc_audio_output_init(sc_audio_output_t *output);

/**
 * Write signed 16-bit interleaved stereo frames with a finite timeout.
 *
 * A successful return guarantees that all requested frames were accepted by
 * the driver. Each frame contains left then right samples. A partial write is
 * reported as ESP_ERR_INVALID_SIZE.
 */
esp_err_t sc_audio_output_write_frames(sc_audio_output_t *output,
                                       const int16_t *interleaved_frames,
                                       size_t frame_count);

/** Disable and release the I2S channel. Safe to call after partial init. */
void sc_audio_output_shutdown(sc_audio_output_t *output);

/** Immediately disable/release output after an unrecoverable diagnostic fault. */
void sc_audio_output_fail_safe(sc_audio_output_t *output);
