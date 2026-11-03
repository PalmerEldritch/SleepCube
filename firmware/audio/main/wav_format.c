#include "wav_format.h"

#include <string.h>

#define SC_WAVE_FORMAT_PCM        0x0001U
#define SC_WAVE_FORMAT_EXTENSIBLE 0xfffeU
#define SC_WAVE_FMT_BASE_SIZE     16U
#define SC_WAVE_FMT_EXT_SIZE      40U
#define SC_WAVE_EXT_MIN_CB_SIZE   22U

static const uint8_t PCM_SUBFORMAT_GUID[16] = {
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00,
    0x10, 0x00,
    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71,
};

static uint16_t read_le16(const uint8_t *value)
{
    return (uint16_t)value[0] | ((uint16_t)value[1] << 8);
}

static uint32_t read_le32(const uint8_t *value)
{
    return (uint32_t)value[0] |
           ((uint32_t)value[1] << 8) |
           ((uint32_t)value[2] << 16) |
           ((uint32_t)value[3] << 24);
}

bool sc_wav_format_parse(const uint8_t *data,
                         size_t data_size,
                         sc_wav_format_t *format)
{
    if (data == NULL || format == NULL || data_size < SC_WAVE_FMT_BASE_SIZE) {
        return false;
    }

    *format = (sc_wav_format_t){
        .format_tag = read_le16(&data[0]),
        .channel_count = read_le16(&data[2]),
        .sample_rate_hz = read_le32(&data[4]),
        .byte_rate = read_le32(&data[8]),
        .block_align = read_le16(&data[12]),
        .container_bits = read_le16(&data[14]),
        .valid_bits = read_le16(&data[14]),
    };

    if (format->format_tag == SC_WAVE_FORMAT_PCM) {
        format->integer_pcm = true;
        return true;
    }
    if (format->format_tag != SC_WAVE_FORMAT_EXTENSIBLE ||
        data_size < SC_WAVE_FMT_EXT_SIZE ||
        read_le16(&data[16]) < SC_WAVE_EXT_MIN_CB_SIZE) {
        return true;
    }

    format->valid_bits = read_le16(&data[18]);
    format->integer_pcm =
        memcmp(&data[24], PCM_SUBFORMAT_GUID, sizeof(PCM_SUBFORMAT_GUID)) == 0;
    return true;
}
