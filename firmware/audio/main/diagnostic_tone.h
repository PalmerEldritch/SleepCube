#pragma once

#include <stddef.h>

#include "audio_output.h"
#include "esp_err.h"

/**
 * Play the finite M1 sequence: silence, 440 Hz, silence, 1 kHz, silence.
 * Both tones use 250 ms linear gain ramps, duplicated left/right samples and
 * a bounded -24 dBFS peak.
 */
esp_err_t sc_diagnostic_tone_play(sc_audio_output_t *output,
                                  size_t *frames_written);
