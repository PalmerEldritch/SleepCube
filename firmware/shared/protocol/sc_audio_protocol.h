#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sc_frame.h"

#define SC_AUDIO_SEMANTIC_VERSION 1U
#define SC_AUDIO_CAP_FINITE_PLAY  (1U << 0)
#define SC_AUDIO_CAP_VOLUME       (1U << 1)
#define SC_AUDIO_CAP_STATUS       (1U << 2)
#define SC_AUDIO_CAP_DEBUG_SOURCE (1U << 3)

#define SC_AUDIO_STATUS_FLAG_SOURCE_MASK        0x07U
#define SC_AUDIO_STATUS_FLAG_SLOT_32            (1U << 3)
#define SC_AUDIO_STATUS_FLAG_NOTE_SHIFT         4U
#define SC_AUDIO_STATUS_FLAG_NOTE_MASK          (7U << SC_AUDIO_STATUS_FLAG_NOTE_SHIFT)
#define SC_AUDIO_STATUS_FLAG_DEBUG_CONFIG_VALID (1U << 7)

typedef enum {
    SC_AUDIO_SOURCE_MUSIC = 0,
    SC_AUDIO_SOURCE_TONE = 1,
    SC_AUDIO_SOURCE_TEST_44100_16 = 2,
    SC_AUDIO_SOURCE_TEST_44100_24 = 3,
    SC_AUDIO_SOURCE_TEST_48000_16 = 4,
    SC_AUDIO_SOURCE_TEST_48000_24 = 5,
    SC_AUDIO_SOURCE_TEST_32000_16 = 6,
    SC_AUDIO_SOURCE_TEST_96000_16 = 7,
    SC_AUDIO_SOURCE_COUNT,
} sc_audio_source_t;

typedef enum {
    SC_AUDIO_I2S_PACKING_NATIVE = 0,
    SC_AUDIO_I2S_PACKING_PADDED_32 = 1,
    SC_AUDIO_I2S_PACKING_COUNT,
} sc_audio_i2s_packing_t;

typedef enum {
    SC_AUDIO_TONE_C3 = 0,
    SC_AUDIO_TONE_C4,
    SC_AUDIO_TONE_C5,
    SC_AUDIO_TONE_C6,
    SC_AUDIO_TONE_C7,
    SC_AUDIO_TONE_C8,
    SC_AUDIO_TONE_COUNT,
} sc_audio_tone_note_t;

typedef enum {
    SC_AUDIO_MSG_HELLO = 0x01,
    SC_AUDIO_MSG_GET_STATUS = 0x02,
    SC_AUDIO_MSG_PLAY = 0x03,
    SC_AUDIO_MSG_STOP = 0x04,
    SC_AUDIO_MSG_SET_VOLUME = 0x05,
    SC_AUDIO_MSG_SET_SOURCE = 0x06,
    SC_AUDIO_MSG_SET_TONE_NOTE = 0x07,
    SC_AUDIO_MSG_SET_I2S_PACKING = 0x08,

    SC_AUDIO_MSG_INFO = 0x81,
    SC_AUDIO_MSG_RESULT = 0x82,
    SC_AUDIO_MSG_READY = 0x83,
    SC_AUDIO_MSG_STATUS = 0x84,
    SC_AUDIO_MSG_PLAYING = 0x85,
    SC_AUDIO_MSG_STOPPED = 0x86,
    SC_AUDIO_MSG_TIMER_EXPIRED = 0x87,
    SC_AUDIO_MSG_ERROR = 0x8f,
} sc_audio_message_type_t;

typedef enum {
    SC_AUDIO_STATE_NOT_READY = 0,
    SC_AUDIO_STATE_STOPPED = 1,
    SC_AUDIO_STATE_STARTING = 2,
    SC_AUDIO_STATE_PLAYING = 3,
    SC_AUDIO_STATE_STOPPING = 4,
    SC_AUDIO_STATE_FAULT = 5,
} sc_audio_state_t;

typedef enum {
    SC_AUDIO_STOP_REASON_BOOT = 0,
    SC_AUDIO_STOP_REASON_MANUAL = 1,
    SC_AUDIO_STOP_REASON_TIMER_EXPIRED = 2,
    SC_AUDIO_STOP_REASON_FAULT = 3,
} sc_audio_stop_reason_t;

typedef enum {
    SC_AUDIO_RESULT_ACCEPTED = 0,
    SC_AUDIO_RESULT_NO_CHANGE = 1,
    SC_AUDIO_RESULT_DUPLICATE = 2,
    SC_AUDIO_RESULT_NOT_READY = 3,
    SC_AUDIO_RESULT_INVALID_ARGUMENT = 4,
    SC_AUDIO_RESULT_SESSION_MISMATCH = 5,
    SC_AUDIO_RESULT_STALE_COMMAND = 6,
    SC_AUDIO_RESULT_COMMAND_ID_REUSED = 7,
    SC_AUDIO_RESULT_BUSY = 8,
    SC_AUDIO_RESULT_INTERNAL_ERROR = 9,
    SC_AUDIO_RESULT_UNSUPPORTED = 10,
    SC_AUDIO_RESULT_MALFORMED = 11,
} sc_audio_result_t;

typedef struct {
    uint32_t audio_session_id;
    sc_audio_state_t state;
    uint8_t volume_percent;
    sc_audio_stop_reason_t stop_reason;
    uint8_t flags;
    uint32_t remaining_seconds;
    uint16_t fault_code;
} sc_audio_status_t;

typedef struct {
    uint32_t control_session_id;
    uint32_t audio_session_id;
    uint8_t semantic_version;
    uint16_t capabilities;
    uint32_t minimum_duration_seconds;
    uint32_t maximum_duration_seconds;
} sc_audio_info_t;

typedef sc_audio_result_t (*sc_audio_get_status_fn)(void *context,
                                                    sc_audio_status_t *status);
typedef sc_audio_result_t (*sc_audio_play_fn)(void *context,
                                              uint32_t duration_seconds);
typedef sc_audio_result_t (*sc_audio_stop_fn)(void *context);
typedef sc_audio_result_t (*sc_audio_set_volume_fn)(void *context,
                                                    uint8_t volume_percent);
typedef sc_audio_result_t (*sc_audio_set_source_fn)(void *context,
                                                    sc_audio_source_t source);
typedef sc_audio_result_t (*sc_audio_set_tone_note_fn)(void *context,
                                                       sc_audio_tone_note_t note);
typedef sc_audio_result_t (*sc_audio_set_i2s_packing_fn)(
    void *context, sc_audio_i2s_packing_t packing);

typedef struct {
    void *context;
    sc_audio_get_status_fn get_status;
    sc_audio_play_fn play;
    sc_audio_stop_fn stop;
    sc_audio_set_volume_fn set_volume;
    sc_audio_set_source_fn set_source;
    sc_audio_set_tone_note_fn set_tone_note;
    sc_audio_set_i2s_packing_fn set_i2s_packing;
} sc_audio_backend_t;

typedef struct {
    uint32_t audio_session_id;
    uint32_t control_session_id;
    uint32_t minimum_duration_seconds;
    uint32_t maximum_duration_seconds;
    uint32_t last_command_id;
    uint8_t last_request_type;
    uint8_t last_payload_len;
    uint8_t last_payload[SC_FRAME_MAX_PAYLOAD];
    sc_audio_result_t last_result;
    bool control_session_valid;
    bool last_command_valid;
    sc_audio_backend_t backend;
} sc_audio_server_t;

void sc_audio_server_init(sc_audio_server_t *server,
                          uint32_t audio_session_id,
                          uint32_t minimum_duration_seconds,
                          uint32_t maximum_duration_seconds,
                          const sc_audio_backend_t *backend);

/** Handle one semantic request and produce exactly one correlated response. */
bool sc_audio_server_handle(sc_audio_server_t *server,
                            const sc_frame_t *request,
                            sc_frame_t *response);

bool sc_audio_make_hello(sc_frame_t *frame,
                         uint16_t sequence,
                         uint32_t control_session_id);
bool sc_audio_make_get_status(sc_frame_t *frame, uint16_t sequence);
bool sc_audio_make_play(sc_frame_t *frame,
                        uint16_t sequence,
                        uint32_t control_session_id,
                        uint32_t audio_session_id,
                        uint32_t command_id,
                        uint32_t duration_seconds);
bool sc_audio_make_stop(sc_frame_t *frame,
                        uint16_t sequence,
                        uint32_t control_session_id,
                        uint32_t audio_session_id,
                        uint32_t command_id);
bool sc_audio_make_set_volume(sc_frame_t *frame,
                              uint16_t sequence,
                              uint32_t control_session_id,
                              uint32_t audio_session_id,
                              uint32_t command_id,
                              uint8_t volume_percent);
bool sc_audio_make_set_source(sc_frame_t *frame,
                              uint16_t sequence,
                              uint32_t control_session_id,
                              uint32_t audio_session_id,
                              uint32_t command_id,
                              sc_audio_source_t source);
bool sc_audio_make_set_tone_note(sc_frame_t *frame,
                                 uint16_t sequence,
                                 uint32_t control_session_id,
                                 uint32_t audio_session_id,
                                 uint32_t command_id,
                                 sc_audio_tone_note_t note);
bool sc_audio_make_set_i2s_packing(sc_frame_t *frame,
                                   uint16_t sequence,
                                   uint32_t control_session_id,
                                   uint32_t audio_session_id,
                                   uint32_t command_id,
                                   sc_audio_i2s_packing_t packing);

uint8_t sc_audio_debug_status_flags(sc_audio_source_t source,
                                    sc_audio_tone_note_t note,
                                    sc_audio_i2s_packing_t packing);
bool sc_audio_debug_config_from_status(const sc_audio_status_t *status,
                                       sc_audio_source_t *source,
                                       sc_audio_tone_note_t *note,
                                       sc_audio_i2s_packing_t *packing);

bool sc_audio_parse_info(const sc_frame_t *frame, sc_audio_info_t *info);
bool sc_audio_parse_status(const sc_frame_t *frame, sc_audio_status_t *status);
bool sc_audio_parse_result(const sc_frame_t *frame,
                           uint8_t *request_type,
                           sc_audio_result_t *result,
                           uint32_t *command_id,
                           sc_audio_status_t *status);

bool sc_audio_status_equal(const sc_audio_status_t *left,
                           const sc_audio_status_t *right);
uint8_t sc_audio_event_type(const sc_audio_status_t *previous,
                            const sc_audio_status_t *current,
                            bool first_report);
bool sc_audio_make_status_event(sc_frame_t *frame,
                                uint8_t event_type,
                                const sc_audio_status_t *status);

const char *sc_audio_message_name(uint8_t type);
const char *sc_audio_state_name(sc_audio_state_t state);
const char *sc_audio_result_name(sc_audio_result_t result);

