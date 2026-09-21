#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SC_FRAME_VERSION 1U
#define SC_FRAME_MAX_PAYLOAD 32U
#define SC_FRAME_MAX_RAW (5U + SC_FRAME_MAX_PAYLOAD + 2U)
#define SC_FRAME_MAX_ENCODED (SC_FRAME_MAX_RAW + 1U)
#define SC_FRAME_MAX_WIRE (SC_FRAME_MAX_ENCODED + 1U)
#define SC_FRAME_INTERBYTE_TIMEOUT_MS 100U

#define SC_FRAME_TYPE_M3_PING 0x70U
#define SC_FRAME_TYPE_M3_PONG 0x71U

typedef struct {
    uint8_t type;
    uint16_t sequence;
    uint8_t payload_len;
    uint8_t payload[SC_FRAME_MAX_PAYLOAD];
} sc_frame_t;

typedef enum {
    SC_FRAME_NONE = 0,
    SC_FRAME_READY,
    SC_FRAME_BAD_COBS,
    SC_FRAME_BAD_LENGTH,
    SC_FRAME_BAD_VERSION,
    SC_FRAME_BAD_CRC,
    SC_FRAME_TOO_LONG,
    SC_FRAME_TIMEOUT,
} sc_frame_result_t;

typedef struct {
    uint8_t encoded[SC_FRAME_MAX_ENCODED];
    size_t encoded_len;
    uint32_t last_byte_ms;
    bool discard_until_delimiter;
} sc_frame_parser_t;

/** Encode one complete packet, including its 0x00 wire delimiter. */
bool sc_frame_encode(const sc_frame_t *frame,
                     uint8_t wire[SC_FRAME_MAX_WIRE],
                     size_t *wire_len);

/** Decode one COBS packet without its 0x00 delimiter. */
sc_frame_result_t sc_frame_decode(const uint8_t *encoded,
                                  size_t encoded_len,
                                  sc_frame_t *frame);

void sc_frame_parser_init(sc_frame_parser_t *parser);

/** Feed one wire byte. Only SC_FRAME_READY makes frame contents valid. */
sc_frame_result_t sc_frame_parser_feed(sc_frame_parser_t *parser,
                                       uint8_t byte,
                                       uint32_t now_ms,
                                       sc_frame_t *frame);

/** Discard a partial packet after a gap; resume only after the next delimiter. */
sc_frame_result_t sc_frame_parser_expire(sc_frame_parser_t *parser,
                                         uint32_t now_ms);
