#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sc_frame.h"

static size_t encode(const sc_frame_t *frame, uint8_t wire[SC_FRAME_MAX_WIRE])
{
    size_t length = 0;
    assert(sc_frame_encode(frame, wire, &length));
    assert(length <= SC_FRAME_MAX_WIRE);
    assert(wire[length - 1U] == 0U);
    return length;
}

static void round_trip(void)
{
    sc_frame_t input = {
        .type = SC_FRAME_TYPE_M3_PING,
        .sequence = 0x1200U,
        .payload_len = SC_FRAME_MAX_PAYLOAD,
    };
    for (size_t i = 0; i < sizeof(input.payload); ++i) {
        input.payload[i] = (uint8_t)i;
    }
    uint8_t wire[SC_FRAME_MAX_WIRE];
    const size_t length = encode(&input, wire);
    sc_frame_t output = {0};
    assert(sc_frame_decode(wire, length - 1U, &output) == SC_FRAME_READY);
    assert(output.type == input.type);
    assert(output.sequence == input.sequence);
    assert(output.payload_len == input.payload_len);
    assert(memcmp(output.payload, input.payload, input.payload_len) == 0);
}

static void invalid_packets(void)
{
    sc_frame_t input = {.type = SC_FRAME_TYPE_M3_PING, .sequence = 7U};
    uint8_t wire[SC_FRAME_MAX_WIRE];
    const size_t length = encode(&input, wire);
    sc_frame_t output = {0};

    assert(sc_frame_decode(wire, 0, &output) == SC_FRAME_BAD_LENGTH);
    uint8_t broken[SC_FRAME_MAX_WIRE];
    memcpy(broken, wire, length);
    broken[0] = 0U;
    assert(sc_frame_decode(broken, length - 1U, &output) == SC_FRAME_BAD_COBS);
    memcpy(broken, wire, length);
    broken[1] = 2U; /* Supported framing, unsupported protocol version. */
    assert(sc_frame_decode(broken, length - 1U, &output) == SC_FRAME_BAD_VERSION);
    memcpy(broken, wire, length);
    broken[length - 2U] ^= 0x20U;
    assert(sc_frame_decode(broken, length - 1U, &output) == SC_FRAME_BAD_CRC);
    assert(sc_frame_decode(wire, length - 2U, &output) == SC_FRAME_BAD_COBS);
    input.payload_len = 1U;
    input.payload[0] = 0x7fU;
    const size_t payload_length = encode(&input, wire);
    assert(wire[5] == 1U); /* This zero-free payload length follows COBS code. */
    wire[5] = 2U;
    assert(sc_frame_decode(wire, payload_length - 1U, &output) ==
           SC_FRAME_BAD_LENGTH);
    input.payload_len = SC_FRAME_MAX_PAYLOAD + 1U;
    size_t ignored = 0;
    assert(!sc_frame_encode(&input, wire, &ignored));
}

static void parser_recovery(void)
{
    sc_frame_t input = {.type = SC_FRAME_TYPE_M3_PONG, .sequence = 42U};
    uint8_t wire[SC_FRAME_MAX_WIRE];
    const size_t length = encode(&input, wire);
    sc_frame_t output = {0};
    sc_frame_parser_t parser;
    sc_frame_parser_init(&parser);

    for (size_t i = 0; i < length / 2U; ++i) {
        assert(sc_frame_parser_feed(&parser, wire[i], 1U, &output) == SC_FRAME_NONE);
    }
    assert(sc_frame_parser_expire(&parser, 102U) == SC_FRAME_TIMEOUT);
    for (size_t i = length / 2U; i < length; ++i) {
        assert(sc_frame_parser_feed(&parser, wire[i], 103U, &output) == SC_FRAME_NONE);
    }
    for (size_t i = 0; i < length - 1U; ++i) {
        assert(sc_frame_parser_feed(&parser, wire[i], 104U, &output) == SC_FRAME_NONE);
    }
    assert(sc_frame_parser_feed(&parser, wire[length - 1U], 104U, &output) ==
           SC_FRAME_READY);
    assert(output.sequence == input.sequence);

    for (size_t i = 0; i < SC_FRAME_MAX_ENCODED; ++i) {
        assert(sc_frame_parser_feed(&parser, 1U, 200U, &output) == SC_FRAME_NONE);
    }
    assert(sc_frame_parser_feed(&parser, 1U, 200U, &output) == SC_FRAME_TOO_LONG);
    assert(sc_frame_parser_feed(&parser, 0U, 201U, &output) == SC_FRAME_NONE);
    for (size_t i = 0; i < length - 1U; ++i) {
        assert(sc_frame_parser_feed(&parser, wire[i], 202U, &output) == SC_FRAME_NONE);
    }
    assert(sc_frame_parser_feed(&parser, 0U, 202U, &output) == SC_FRAME_READY);
}

int main(void)
{
    round_trip();
    invalid_packets();
    parser_recovery();
    puts("sc_frame tests passed");
    return 0;
}
