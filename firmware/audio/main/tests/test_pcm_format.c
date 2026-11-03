#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pcm_format.h"

static void decode_preserves_16_and_24_bits(void)
{
    const uint8_t zero[] = {0x00, 0x00, 0x00};
    const uint8_t max16[] = {0xff, 0x7f};
    const uint8_t min16[] = {0x00, 0x80};
    const uint8_t one24[] = {0x01, 0x00, 0x00};
    const uint8_t max24[] = {0xff, 0xff, 0x7f};
    const uint8_t min24[] = {0x00, 0x00, 0x80};
    assert(sc_pcm_le_sample_to_q31(zero, 16U) == 0);
    assert(sc_pcm_le_sample_to_q31(max16, 16U) == 0x7fff0000);
    assert(sc_pcm_le_sample_to_q31(min16, 16U) == INT32_MIN);
    assert(sc_pcm_le_sample_to_q31(one24, 24U) == 0x00000100);
    assert(sc_pcm_le_sample_to_q31(max24, 24U) == 0x7fffff00);
    assert(sc_pcm_le_sample_to_q31(min24, 24U) == INT32_MIN);
}

static void pack_quantizes_and_duplicates_channels(void)
{
    const int32_t frames[] = {0x12345678, (int32_t)0x89abcdefU};
    uint8_t packed[12] = {0};
    const uint8_t expected16[] = {
        0x34, 0x12, 0x34, 0x12, 0xab, 0x89, 0xab, 0x89,
    };
    assert(sc_pcm_pack_stereo_q31(frames, 2U, 16U, packed,
                                  sizeof(packed)) == sizeof(expected16));
    assert(memcmp(packed, expected16, sizeof(expected16)) == 0);

    const uint8_t expected24[] = {
        0x56, 0x34, 0x12, 0x56, 0x34, 0x12,
        0xcd, 0xab, 0x89, 0xcd, 0xab, 0x89,
    };
    memset(packed, 0, sizeof(packed));
    assert(sc_pcm_pack_stereo_q31(frames, 2U, 24U, packed,
                                  sizeof(packed)) == sizeof(expected24));
    assert(memcmp(packed, expected24, sizeof(expected24)) == 0);
    assert(sc_pcm_pack_stereo_q31(frames, 2U, 24U, packed, 11U) == 0U);
}

int main(void)
{
    decode_preserves_16_and_24_bits();
    pack_quantizes_and_duplicates_channels();
    puts("pcm_format tests passed");
    return 0;
}

