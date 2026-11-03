#include <assert.h>
#include <stdio.h>

#include "wav_format.h"

static void classic_pcm_is_accepted(void)
{
    const uint8_t fmt[16] = {
        0x01, 0x00, 0x01, 0x00,
        0x44, 0xac, 0x00, 0x00,
        0x88, 0x58, 0x01, 0x00,
        0x02, 0x00, 0x10, 0x00,
    };
    sc_wav_format_t parsed;
    assert(sc_wav_format_parse(fmt, sizeof(fmt), &parsed));
    assert(parsed.integer_pcm);
    assert(parsed.format_tag == 1U);
    assert(parsed.channel_count == 1U);
    assert(parsed.sample_rate_hz == 44100U);
    assert(parsed.byte_rate == 88200U);
    assert(parsed.block_align == 2U);
    assert(parsed.container_bits == 16U);
    assert(parsed.valid_bits == 16U);
}

static void extensible_pcm24_is_accepted(void)
{
    const uint8_t fmt[40] = {
        0xfe, 0xff, 0x01, 0x00,
        0x44, 0xac, 0x00, 0x00,
        0xcc, 0x04, 0x02, 0x00,
        0x03, 0x00, 0x18, 0x00,
        0x16, 0x00, 0x18, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x10, 0x00,
        0x80, 0x00, 0x00, 0xaa,
        0x00, 0x38, 0x9b, 0x71,
    };
    sc_wav_format_t parsed;
    assert(sc_wav_format_parse(fmt, sizeof(fmt), &parsed));
    assert(parsed.integer_pcm);
    assert(parsed.format_tag == 0xfffeU);
    assert(parsed.channel_count == 1U);
    assert(parsed.sample_rate_hz == 44100U);
    assert(parsed.byte_rate == 132300U);
    assert(parsed.block_align == 3U);
    assert(parsed.container_bits == 24U);
    assert(parsed.valid_bits == 24U);
}

static void extensible_float_and_truncation_are_rejected(void)
{
    uint8_t fmt[40] = {
        0xfe, 0xff, 0x01, 0x00,
        0x80, 0xbb, 0x00, 0x00,
        0x00, 0xee, 0x02, 0x00,
        0x04, 0x00, 0x20, 0x00,
        0x16, 0x00, 0x20, 0x00,
        0x04, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x10, 0x00,
        0x80, 0x00, 0x00, 0xaa,
        0x00, 0x38, 0x9b, 0x71,
    };
    sc_wav_format_t parsed;
    assert(sc_wav_format_parse(fmt, sizeof(fmt), &parsed));
    assert(!parsed.integer_pcm);
    assert(!sc_wav_format_parse(fmt, 15U, &parsed));

    fmt[16] = 0x10;
    assert(sc_wav_format_parse(fmt, sizeof(fmt), &parsed));
    assert(!parsed.integer_pcm);
}

int main(void)
{
    classic_pcm_is_accepted();
    extensible_pcm24_is_accepted();
    extensible_float_and_truncation_are_rejected();
    puts("wav_format tests passed");
    return 0;
}
