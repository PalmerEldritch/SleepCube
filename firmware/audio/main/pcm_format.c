#include "pcm_format.h"

int32_t sc_pcm_le_sample_to_q31(const uint8_t *sample, uint8_t data_bits)
{
    if (sample == NULL) {
        return 0;
    }
    if (data_bits == 16U) {
        const uint16_t raw = (uint16_t)sample[0] |
                             ((uint16_t)sample[1] << 8);
        return (int32_t)(int16_t)raw * 65536;
    }
    if (data_bits == 24U) {
        uint32_t raw = (uint32_t)sample[0] |
                       ((uint32_t)sample[1] << 8) |
                       ((uint32_t)sample[2] << 16);
        if ((raw & 0x00800000U) != 0U) {
            raw |= 0xff000000U;
        }
        return (int32_t)raw * 256;
    }
    return 0;
}

size_t sc_pcm_pack_stereo_q31(const int32_t *mono_frames,
                              size_t frame_count,
                              uint8_t data_bits,
                              uint8_t *packed,
                              size_t packed_capacity)
{
    if ((mono_frames == NULL && frame_count != 0U) || packed == NULL ||
        (data_bits != 16U && data_bits != 24U)) {
        return 0U;
    }
    const size_t bytes_per_sample = data_bits / 8U;
    const size_t requested = frame_count * 2U * bytes_per_sample;
    if (requested > packed_capacity) {
        return 0U;
    }

    size_t offset = 0U;
    for (size_t i = 0; i < frame_count; ++i) {
        const uint32_t value = data_bits == 16U
                                   ? (uint32_t)mono_frames[i] >> 16
                                   : (uint32_t)mono_frames[i] >> 8;
        for (size_t channel = 0; channel < 2U; ++channel) {
            packed[offset++] = (uint8_t)value;
            packed[offset++] = (uint8_t)(value >> 8);
            if (data_bits == 24U) {
                packed[offset++] = (uint8_t)(value >> 16);
            }
        }
    }
    return offset;
}

