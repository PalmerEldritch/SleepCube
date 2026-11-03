#pragma once

#include <stddef.h>
#include <stdint.h>

/** Decode one packed little-endian signed 16/24-bit sample into Q31. */
int32_t sc_pcm_le_sample_to_q31(const uint8_t *sample, uint8_t data_bits);

/**
 * Quantize mono Q31 samples to 16/24 bits and duplicate them into packed
 * little-endian stereo DMA data. Returns zero for invalid arguments.
 */
size_t sc_pcm_pack_stereo_q31(const int32_t *mono_frames,
                              size_t frame_count,
                              uint8_t data_bits,
                              uint8_t *packed,
                              size_t packed_capacity);

