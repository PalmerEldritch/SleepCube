#include "sc_frame.h"

#include <string.h>

static uint16_t crc16_ccitt_false(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xffffU;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8U; ++bit) {
            crc = (crc & 0x8000U) != 0U
                      ? (uint16_t)((crc << 1) ^ 0x1021U)
                      : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

bool sc_frame_encode(const sc_frame_t *frame,
                     uint8_t wire[SC_FRAME_MAX_WIRE],
                     size_t *wire_len)
{
    if (frame == NULL || wire == NULL || wire_len == NULL ||
        frame->payload_len > SC_FRAME_MAX_PAYLOAD) {
        return false;
    }

    uint8_t raw[SC_FRAME_MAX_RAW];
    raw[0] = SC_FRAME_VERSION;
    raw[1] = frame->type;
    raw[2] = (uint8_t)frame->sequence;
    raw[3] = (uint8_t)(frame->sequence >> 8);
    raw[4] = frame->payload_len;
    memcpy(&raw[5], frame->payload, frame->payload_len);
    const size_t content_len = 5U + frame->payload_len;
    const uint16_t crc = crc16_ccitt_false(raw, content_len);
    raw[content_len] = (uint8_t)crc;
    raw[content_len + 1U] = (uint8_t)(crc >> 8);
    const size_t raw_len = content_len + 2U;

    size_t code_index = 0;
    size_t output_index = 1;
    uint8_t code = 1;
    for (size_t i = 0; i < raw_len; ++i) {
        if (raw[i] == 0U) {
            wire[code_index] = code;
            code_index = output_index++;
            code = 1;
        } else {
            wire[output_index++] = raw[i];
            ++code;
        }
    }
    wire[code_index] = code;
    wire[output_index++] = 0U;
    *wire_len = output_index;
    return output_index <= SC_FRAME_MAX_WIRE;
}

sc_frame_result_t sc_frame_decode(const uint8_t *encoded,
                                  size_t encoded_len,
                                  sc_frame_t *frame)
{
    if (encoded == NULL || frame == NULL || encoded_len == 0U ||
        encoded_len > SC_FRAME_MAX_ENCODED) {
        return SC_FRAME_BAD_LENGTH;
    }

    uint8_t raw[SC_FRAME_MAX_RAW];
    size_t input_index = 0;
    size_t raw_len = 0;
    while (input_index < encoded_len) {
        const uint8_t code = encoded[input_index++];
        if (code == 0U || (size_t)(code - 1U) > encoded_len - input_index) {
            return SC_FRAME_BAD_COBS;
        }
        for (uint16_t i = 1U; i < code; ++i) {
            if (raw_len >= sizeof(raw)) {
                return SC_FRAME_BAD_LENGTH;
            }
            raw[raw_len++] = encoded[input_index++];
        }
        if (code != 0xffU && input_index < encoded_len) {
            if (raw_len >= sizeof(raw)) {
                return SC_FRAME_BAD_LENGTH;
            }
            raw[raw_len++] = 0U;
        }
    }

    if (raw_len < 7U || raw[4] > SC_FRAME_MAX_PAYLOAD ||
        raw_len != 7U + raw[4]) {
        return SC_FRAME_BAD_LENGTH;
    }
    if (raw[0] != SC_FRAME_VERSION) {
        return SC_FRAME_BAD_VERSION;
    }
    const uint16_t expected_crc = crc16_ccitt_false(raw, raw_len - 2U);
    const uint16_t actual_crc = (uint16_t)raw[raw_len - 2U] |
                                ((uint16_t)raw[raw_len - 1U] << 8);
    if (expected_crc != actual_crc) {
        return SC_FRAME_BAD_CRC;
    }

    frame->type = raw[1];
    frame->sequence = (uint16_t)raw[2] | ((uint16_t)raw[3] << 8);
    frame->payload_len = raw[4];
    memcpy(frame->payload, &raw[5], frame->payload_len);
    return SC_FRAME_READY;
}

void sc_frame_parser_init(sc_frame_parser_t *parser)
{
    if (parser != NULL) {
        memset(parser, 0, sizeof(*parser));
    }
}

sc_frame_result_t sc_frame_parser_expire(sc_frame_parser_t *parser,
                                         uint32_t now_ms)
{
    if (parser == NULL || parser->discard_until_delimiter ||
        parser->encoded_len == 0U ||
        (uint32_t)(now_ms - parser->last_byte_ms) <=
            SC_FRAME_INTERBYTE_TIMEOUT_MS) {
        return SC_FRAME_NONE;
    }
    parser->encoded_len = 0U;
    parser->discard_until_delimiter = true;
    return SC_FRAME_TIMEOUT;
}

sc_frame_result_t sc_frame_parser_feed(sc_frame_parser_t *parser,
                                       uint8_t byte,
                                       uint32_t now_ms,
                                       sc_frame_t *frame)
{
    if (parser == NULL || frame == NULL) {
        return SC_FRAME_BAD_LENGTH;
    }

    const sc_frame_result_t expired = sc_frame_parser_expire(parser, now_ms);
    if (byte == 0U) {
        if (parser->discard_until_delimiter) {
            parser->discard_until_delimiter = false;
            parser->encoded_len = 0U;
            return expired;
        }
        if (parser->encoded_len == 0U) {
            return expired;
        }
        const sc_frame_result_t result = sc_frame_decode(parser->encoded,
                                                         parser->encoded_len,
                                                         frame);
        parser->encoded_len = 0U;
        return result;
    }
    if (expired != SC_FRAME_NONE || parser->discard_until_delimiter) {
        return expired;
    }
    if (parser->encoded_len >= SC_FRAME_MAX_ENCODED) {
        parser->encoded_len = 0U;
        parser->discard_until_delimiter = true;
        return SC_FRAME_TOO_LONG;
    }
    parser->encoded[parser->encoded_len++] = byte;
    parser->last_byte_ms = now_ms;
    return SC_FRAME_NONE;
}
