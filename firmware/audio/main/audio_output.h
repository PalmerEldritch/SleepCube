#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2s_std.h"
#include "esp_err.h"

typedef struct {
    i2s_chan_handle_t tx_channel;
    bool initialized;
    uint32_t sample_rate_hz;
    uint8_t data_bits;
    uint8_t slot_bits;
} sc_audio_output_t;

/** Initialize and enable the AtomS3 Lite I2S output in a silent state. */
esp_err_t sc_audio_output_init(sc_audio_output_t *output);

/**
 * Recreate the enabled I2S transmitter with the requested diagnostic format.
 * Supported data widths are 16 and 24 bits; slots may equal the data width or
 * be padded to 32 bits. The output returns in the driver's silent auto-clear
 * state and remains clocked while playback is stopped.
 */
esp_err_t sc_audio_output_reconfigure(sc_audio_output_t *output,
                                      uint32_t sample_rate_hz,
                                      uint8_t data_bits,
                                      uint8_t slot_bits);

/**
 * Write signed mono Q31 samples, duplicated into left and right I2S slots,
 * with a finite timeout. The configured 16- or 24-bit data width determines
 * the DMA representation; 24-bit source precision is retained.
 *
 * A successful return guarantees that all requested frames were accepted by
 * the driver. A partial write is reported as ESP_ERR_INVALID_SIZE.
 */
esp_err_t sc_audio_output_write_mono_q31(sc_audio_output_t *output,
                                         const int32_t *mono_frames,
                                         size_t frame_count);

/** DMA bytes accepted per stereo frame in the active data-width format. */
size_t sc_audio_output_bytes_per_frame(const sc_audio_output_t *output);

/** Disable and release the I2S channel. Safe to call after partial init. */
void sc_audio_output_shutdown(sc_audio_output_t *output);

/** Immediately disable/release output after an unrecoverable diagnostic fault. */
void sc_audio_output_fail_safe(sc_audio_output_t *output);
