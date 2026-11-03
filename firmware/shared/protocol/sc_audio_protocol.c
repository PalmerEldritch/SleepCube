#include "sc_audio_protocol.h"

#include <string.h>

#define SC_AUDIO_STATUS_PAYLOAD_SIZE 14U
#define SC_AUDIO_COMMAND_HEADER_SIZE 12U
#define SC_AUDIO_RESULT_PAYLOAD_SIZE (6U + SC_AUDIO_STATUS_PAYLOAD_SIZE)

static uint16_t get_u16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint32_t get_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static void put_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static bool valid_status(const sc_audio_status_t *status)
{
    return status != NULL && status->audio_session_id != 0U &&
           status->state <= SC_AUDIO_STATE_FAULT &&
           status->volume_percent <= 100U &&
           status->stop_reason <= SC_AUDIO_STOP_REASON_FAULT;
}

static void encode_status_payload(uint8_t *payload,
                                  const sc_audio_status_t *status)
{
    put_u32(&payload[0], status->audio_session_id);
    payload[4] = (uint8_t)status->state;
    payload[5] = status->volume_percent;
    payload[6] = (uint8_t)status->stop_reason;
    payload[7] = status->flags;
    put_u32(&payload[8], status->remaining_seconds);
    put_u16(&payload[12], status->fault_code);
}

static bool decode_status_payload(const uint8_t *payload,
                                  size_t payload_len,
                                  sc_audio_status_t *status)
{
    if (payload == NULL || payload_len != SC_AUDIO_STATUS_PAYLOAD_SIZE ||
        status == NULL) {
        return false;
    }
    status->audio_session_id = get_u32(&payload[0]);
    status->state = (sc_audio_state_t)payload[4];
    status->volume_percent = payload[5];
    status->stop_reason = (sc_audio_stop_reason_t)payload[6];
    status->flags = payload[7];
    status->remaining_seconds = get_u32(&payload[8]);
    status->fault_code = get_u16(&payload[12]);
    return valid_status(status);
}

static sc_audio_status_t get_status(sc_audio_server_t *server)
{
    sc_audio_status_t status = {
        .audio_session_id = server->audio_session_id,
        .state = SC_AUDIO_STATE_NOT_READY,
        .stop_reason = SC_AUDIO_STOP_REASON_BOOT,
    };
    sc_audio_result_t result = SC_AUDIO_RESULT_ACCEPTED;
    if (server->backend.get_status != NULL) {
        result = server->backend.get_status(server->backend.context, &status);
    }
    status.audio_session_id = server->audio_session_id;
    if (result != SC_AUDIO_RESULT_ACCEPTED || !valid_status(&status)) {
        status.audio_session_id = server->audio_session_id;
        status.state = SC_AUDIO_STATE_FAULT;
        status.volume_percent = 0U;
        status.stop_reason = SC_AUDIO_STOP_REASON_FAULT;
        status.flags = 0U;
        status.remaining_seconds = 0U;
        status.fault_code = 1U;
    }
    return status;
}

static void make_error(sc_audio_server_t *server,
                       const sc_frame_t *request,
                       sc_audio_result_t error,
                       sc_frame_t *response)
{
    const sc_audio_status_t status = get_status(server);
    memset(response, 0, sizeof(*response));
    response->type = SC_AUDIO_MSG_ERROR;
    response->sequence = request->sequence;
    response->payload_len = 2U + SC_AUDIO_STATUS_PAYLOAD_SIZE;
    response->payload[0] = request->type;
    response->payload[1] = (uint8_t)error;
    encode_status_payload(&response->payload[2], &status);
}

static void make_result(sc_audio_server_t *server,
                        const sc_frame_t *request,
                        sc_audio_result_t result,
                        uint32_t command_id,
                        sc_frame_t *response)
{
    const sc_audio_status_t status = get_status(server);
    memset(response, 0, sizeof(*response));
    response->type = SC_AUDIO_MSG_RESULT;
    response->sequence = request->sequence;
    response->payload_len = SC_AUDIO_RESULT_PAYLOAD_SIZE;
    response->payload[0] = request->type;
    response->payload[1] = (uint8_t)result;
    put_u32(&response->payload[2], command_id);
    encode_status_payload(&response->payload[6], &status);
}

static bool same_cached_command(const sc_audio_server_t *server,
                                const sc_frame_t *request)
{
    return server->last_command_valid &&
           server->last_request_type == request->type &&
           server->last_payload_len == request->payload_len &&
           memcmp(server->last_payload, request->payload,
                  request->payload_len) == 0;
}

static void cache_command(sc_audio_server_t *server,
                          const sc_frame_t *request,
                          uint32_t command_id,
                          sc_audio_result_t result)
{
    server->last_command_valid = true;
    server->last_command_id = command_id;
    server->last_request_type = request->type;
    server->last_payload_len = request->payload_len;
    memcpy(server->last_payload, request->payload, request->payload_len);
    server->last_result = result;
}

void sc_audio_server_init(sc_audio_server_t *server,
                          uint32_t audio_session_id,
                          uint32_t minimum_duration_seconds,
                          uint32_t maximum_duration_seconds,
                          const sc_audio_backend_t *backend)
{
    if (server == NULL) {
        return;
    }
    memset(server, 0, sizeof(*server));
    server->audio_session_id = audio_session_id == 0U ? 1U : audio_session_id;
    server->minimum_duration_seconds = minimum_duration_seconds;
    server->maximum_duration_seconds = maximum_duration_seconds;
    if (backend != NULL) {
        server->backend = *backend;
    }
}

static bool handle_hello(sc_audio_server_t *server,
                         const sc_frame_t *request,
                         sc_frame_t *response)
{
    if (request->payload_len != 4U || get_u32(request->payload) == 0U) {
        make_error(server, request, SC_AUDIO_RESULT_MALFORMED, response);
        return true;
    }
    const uint32_t control_session_id = get_u32(request->payload);
    if (!server->control_session_valid ||
        server->control_session_id != control_session_id) {
        server->control_session_id = control_session_id;
        server->control_session_valid = true;
        server->last_command_valid = false;
    }

    memset(response, 0, sizeof(*response));
    response->type = SC_AUDIO_MSG_INFO;
    response->sequence = request->sequence;
    response->payload_len = 19U;
    put_u32(&response->payload[0], control_session_id);
    put_u32(&response->payload[4], server->audio_session_id);
    response->payload[8] = SC_AUDIO_SEMANTIC_VERSION;
    uint16_t capabilities = SC_AUDIO_CAP_FINITE_PLAY |
                            SC_AUDIO_CAP_VOLUME |
                            SC_AUDIO_CAP_STATUS;
    if (server->backend.set_source != NULL &&
        server->backend.set_tone_note != NULL &&
        server->backend.set_i2s_packing != NULL) {
        capabilities |= SC_AUDIO_CAP_DEBUG_SOURCE;
    }
    put_u16(&response->payload[9], capabilities);
    put_u32(&response->payload[11], server->minimum_duration_seconds);
    put_u32(&response->payload[15], server->maximum_duration_seconds);
    return true;
}

static bool handle_get_status(sc_audio_server_t *server,
                              const sc_frame_t *request,
                              sc_frame_t *response)
{
    if (request->payload_len != 0U) {
        make_error(server, request, SC_AUDIO_RESULT_MALFORMED, response);
        return true;
    }
    const sc_audio_status_t status = get_status(server);
    memset(response, 0, sizeof(*response));
    response->type = SC_AUDIO_MSG_STATUS;
    response->sequence = request->sequence;
    response->payload_len = SC_AUDIO_STATUS_PAYLOAD_SIZE;
    encode_status_payload(response->payload, &status);
    return true;
}

static bool handle_command(sc_audio_server_t *server,
                           const sc_frame_t *request,
                           sc_frame_t *response)
{
    const size_t expected_len = request->type == SC_AUDIO_MSG_PLAY
                                    ? 16U
                                    : (request->type == SC_AUDIO_MSG_SET_VOLUME ||
                                       request->type == SC_AUDIO_MSG_SET_SOURCE ||
                                       request->type == SC_AUDIO_MSG_SET_TONE_NOTE ||
                                       request->type == SC_AUDIO_MSG_SET_I2S_PACKING)
                                          ? 13U
                                          : SC_AUDIO_COMMAND_HEADER_SIZE;
    if (request->payload_len != expected_len) {
        make_error(server, request, SC_AUDIO_RESULT_MALFORMED, response);
        return true;
    }
    const uint32_t control_session_id = get_u32(&request->payload[0]);
    const uint32_t audio_session_id = get_u32(&request->payload[4]);
    const uint32_t command_id = get_u32(&request->payload[8]);
    if (!server->control_session_valid ||
        control_session_id != server->control_session_id ||
        audio_session_id != server->audio_session_id) {
        make_result(server, request, SC_AUDIO_RESULT_SESSION_MISMATCH,
                    command_id, response);
        return true;
    }
    if (command_id == 0U) {
        make_result(server, request, SC_AUDIO_RESULT_INVALID_ARGUMENT,
                    command_id, response);
        return true;
    }
    if (server->last_command_valid && command_id == server->last_command_id) {
        make_result(server, request,
                    same_cached_command(server, request)
                        ? SC_AUDIO_RESULT_DUPLICATE
                        : SC_AUDIO_RESULT_COMMAND_ID_REUSED,
                    command_id, response);
        return true;
    }
    if (server->last_command_valid && command_id < server->last_command_id) {
        make_result(server, request, SC_AUDIO_RESULT_STALE_COMMAND,
                    command_id, response);
        return true;
    }

    sc_audio_result_t result = SC_AUDIO_RESULT_NOT_READY;
    if (request->type == SC_AUDIO_MSG_PLAY) {
        const uint32_t duration_seconds = get_u32(&request->payload[12]);
        if (duration_seconds < server->minimum_duration_seconds ||
            duration_seconds > server->maximum_duration_seconds) {
            result = SC_AUDIO_RESULT_INVALID_ARGUMENT;
        } else if (server->backend.play != NULL) {
            result = server->backend.play(server->backend.context,
                                          duration_seconds);
        }
    } else if (request->type == SC_AUDIO_MSG_STOP) {
        if (server->backend.stop != NULL) {
            result = server->backend.stop(server->backend.context);
        }
    } else if (request->type == SC_AUDIO_MSG_SET_VOLUME) {
        const uint8_t volume_percent = request->payload[12];
        if (volume_percent > 100U) {
            result = SC_AUDIO_RESULT_INVALID_ARGUMENT;
        } else if (server->backend.set_volume != NULL) {
            result = server->backend.set_volume(server->backend.context,
                                                volume_percent);
        }
    } else if (request->type == SC_AUDIO_MSG_SET_SOURCE) {
        const uint8_t source = request->payload[12];
        if (source >= SC_AUDIO_SOURCE_COUNT) {
            result = SC_AUDIO_RESULT_INVALID_ARGUMENT;
        } else if (server->backend.set_source != NULL) {
            result = server->backend.set_source(server->backend.context,
                                                (sc_audio_source_t)source);
        } else {
            result = SC_AUDIO_RESULT_UNSUPPORTED;
        }
    } else if (request->type == SC_AUDIO_MSG_SET_TONE_NOTE) {
        const uint8_t note = request->payload[12];
        if (note >= SC_AUDIO_TONE_COUNT) {
            result = SC_AUDIO_RESULT_INVALID_ARGUMENT;
        } else if (server->backend.set_tone_note != NULL) {
            result = server->backend.set_tone_note(server->backend.context,
                                                   (sc_audio_tone_note_t)note);
        } else {
            result = SC_AUDIO_RESULT_UNSUPPORTED;
        }
    } else {
        const uint8_t packing = request->payload[12];
        if (packing >= SC_AUDIO_I2S_PACKING_COUNT) {
            result = SC_AUDIO_RESULT_INVALID_ARGUMENT;
        } else if (server->backend.set_i2s_packing != NULL) {
            result = server->backend.set_i2s_packing(
                server->backend.context, (sc_audio_i2s_packing_t)packing);
        } else {
            result = SC_AUDIO_RESULT_UNSUPPORTED;
        }
    }
    cache_command(server, request, command_id, result);
    make_result(server, request, result, command_id, response);
    return true;
}

bool sc_audio_server_handle(sc_audio_server_t *server,
                            const sc_frame_t *request,
                            sc_frame_t *response)
{
    if (server == NULL || request == NULL || response == NULL) {
        return false;
    }
    switch (request->type) {
    case SC_AUDIO_MSG_HELLO:
        return handle_hello(server, request, response);
    case SC_AUDIO_MSG_GET_STATUS:
        return handle_get_status(server, request, response);
    case SC_AUDIO_MSG_PLAY:
    case SC_AUDIO_MSG_STOP:
    case SC_AUDIO_MSG_SET_VOLUME:
    case SC_AUDIO_MSG_SET_SOURCE:
    case SC_AUDIO_MSG_SET_TONE_NOTE:
    case SC_AUDIO_MSG_SET_I2S_PACKING:
        return handle_command(server, request, response);
    default:
        make_error(server, request, SC_AUDIO_RESULT_UNSUPPORTED, response);
        return true;
    }
}

static void make_command(sc_frame_t *frame,
                         uint8_t type,
                         uint16_t sequence,
                         uint32_t control_session_id,
                         uint32_t audio_session_id,
                         uint32_t command_id)
{
    memset(frame, 0, sizeof(*frame));
    frame->type = type;
    frame->sequence = sequence;
    frame->payload_len = SC_AUDIO_COMMAND_HEADER_SIZE;
    put_u32(&frame->payload[0], control_session_id);
    put_u32(&frame->payload[4], audio_session_id);
    put_u32(&frame->payload[8], command_id);
}

bool sc_audio_make_hello(sc_frame_t *frame,
                         uint16_t sequence,
                         uint32_t control_session_id)
{
    if (frame == NULL || control_session_id == 0U) {
        return false;
    }
    memset(frame, 0, sizeof(*frame));
    frame->type = SC_AUDIO_MSG_HELLO;
    frame->sequence = sequence;
    frame->payload_len = 4U;
    put_u32(frame->payload, control_session_id);
    return true;
}

bool sc_audio_make_get_status(sc_frame_t *frame, uint16_t sequence)
{
    if (frame == NULL) {
        return false;
    }
    memset(frame, 0, sizeof(*frame));
    frame->type = SC_AUDIO_MSG_GET_STATUS;
    frame->sequence = sequence;
    return true;
}

bool sc_audio_make_play(sc_frame_t *frame,
                        uint16_t sequence,
                        uint32_t control_session_id,
                        uint32_t audio_session_id,
                        uint32_t command_id,
                        uint32_t duration_seconds)
{
    if (frame == NULL) {
        return false;
    }
    make_command(frame, SC_AUDIO_MSG_PLAY, sequence, control_session_id,
                 audio_session_id, command_id);
    frame->payload_len = 16U;
    put_u32(&frame->payload[12], duration_seconds);
    return true;
}

bool sc_audio_make_stop(sc_frame_t *frame,
                        uint16_t sequence,
                        uint32_t control_session_id,
                        uint32_t audio_session_id,
                        uint32_t command_id)
{
    if (frame == NULL) {
        return false;
    }
    make_command(frame, SC_AUDIO_MSG_STOP, sequence, control_session_id,
                 audio_session_id, command_id);
    return true;
}

bool sc_audio_make_set_volume(sc_frame_t *frame,
                              uint16_t sequence,
                              uint32_t control_session_id,
                              uint32_t audio_session_id,
                              uint32_t command_id,
                              uint8_t volume_percent)
{
    if (frame == NULL) {
        return false;
    }
    make_command(frame, SC_AUDIO_MSG_SET_VOLUME, sequence, control_session_id,
                 audio_session_id, command_id);
    frame->payload_len = 13U;
    frame->payload[12] = volume_percent;
    return true;
}

bool sc_audio_make_set_source(sc_frame_t *frame,
                              uint16_t sequence,
                              uint32_t control_session_id,
                              uint32_t audio_session_id,
                              uint32_t command_id,
                              sc_audio_source_t source)
{
    if (frame == NULL || source >= SC_AUDIO_SOURCE_COUNT) {
        return false;
    }
    make_command(frame, SC_AUDIO_MSG_SET_SOURCE, sequence, control_session_id,
                 audio_session_id, command_id);
    frame->payload_len = 13U;
    frame->payload[12] = (uint8_t)source;
    return true;
}

bool sc_audio_make_set_tone_note(sc_frame_t *frame,
                                 uint16_t sequence,
                                 uint32_t control_session_id,
                                 uint32_t audio_session_id,
                                 uint32_t command_id,
                                 sc_audio_tone_note_t note)
{
    if (frame == NULL || note >= SC_AUDIO_TONE_COUNT) {
        return false;
    }
    make_command(frame, SC_AUDIO_MSG_SET_TONE_NOTE, sequence,
                 control_session_id, audio_session_id, command_id);
    frame->payload_len = 13U;
    frame->payload[12] = (uint8_t)note;
    return true;
}

bool sc_audio_make_set_i2s_packing(sc_frame_t *frame,
                                   uint16_t sequence,
                                   uint32_t control_session_id,
                                   uint32_t audio_session_id,
                                   uint32_t command_id,
                                   sc_audio_i2s_packing_t packing)
{
    if (frame == NULL || packing >= SC_AUDIO_I2S_PACKING_COUNT) {
        return false;
    }
    make_command(frame, SC_AUDIO_MSG_SET_I2S_PACKING, sequence,
                 control_session_id, audio_session_id, command_id);
    frame->payload_len = 13U;
    frame->payload[12] = (uint8_t)packing;
    return true;
}

uint8_t sc_audio_debug_status_flags(sc_audio_source_t source,
                                    sc_audio_tone_note_t note,
                                    sc_audio_i2s_packing_t packing)
{
    if (source >= SC_AUDIO_SOURCE_COUNT || note >= SC_AUDIO_TONE_COUNT ||
        packing >= SC_AUDIO_I2S_PACKING_COUNT) {
        return 0U;
    }
    return SC_AUDIO_STATUS_FLAG_DEBUG_CONFIG_VALID |
           ((uint8_t)source & SC_AUDIO_STATUS_FLAG_SOURCE_MASK) |
           (packing == SC_AUDIO_I2S_PACKING_PADDED_32
                ? SC_AUDIO_STATUS_FLAG_SLOT_32 : 0U) |
           ((uint8_t)note << SC_AUDIO_STATUS_FLAG_NOTE_SHIFT);
}

bool sc_audio_debug_config_from_status(const sc_audio_status_t *status,
                                       sc_audio_source_t *source,
                                       sc_audio_tone_note_t *note,
                                       sc_audio_i2s_packing_t *packing)
{
    if (status == NULL || source == NULL || note == NULL || packing == NULL ||
        (status->flags & SC_AUDIO_STATUS_FLAG_DEBUG_CONFIG_VALID) == 0U) {
        return false;
    }
    const uint8_t encoded_source =
        status->flags & SC_AUDIO_STATUS_FLAG_SOURCE_MASK;
    const uint8_t encoded_note =
        (status->flags & SC_AUDIO_STATUS_FLAG_NOTE_MASK) >>
        SC_AUDIO_STATUS_FLAG_NOTE_SHIFT;
    if (encoded_source >= SC_AUDIO_SOURCE_COUNT ||
        encoded_note >= SC_AUDIO_TONE_COUNT) {
        return false;
    }
    *source = (sc_audio_source_t)encoded_source;
    *note = (sc_audio_tone_note_t)encoded_note;
    *packing = (status->flags & SC_AUDIO_STATUS_FLAG_SLOT_32) != 0U
                   ? SC_AUDIO_I2S_PACKING_PADDED_32
                   : SC_AUDIO_I2S_PACKING_NATIVE;
    return true;
}

bool sc_audio_parse_info(const sc_frame_t *frame, sc_audio_info_t *info)
{
    if (frame == NULL || info == NULL || frame->type != SC_AUDIO_MSG_INFO ||
        frame->payload_len != 19U) {
        return false;
    }
    info->control_session_id = get_u32(&frame->payload[0]);
    info->audio_session_id = get_u32(&frame->payload[4]);
    info->semantic_version = frame->payload[8];
    info->capabilities = get_u16(&frame->payload[9]);
    info->minimum_duration_seconds = get_u32(&frame->payload[11]);
    info->maximum_duration_seconds = get_u32(&frame->payload[15]);
    return info->control_session_id != 0U && info->audio_session_id != 0U;
}

bool sc_audio_parse_status(const sc_frame_t *frame, sc_audio_status_t *status)
{
    if (frame == NULL || (frame->type != SC_AUDIO_MSG_READY &&
                          frame->type != SC_AUDIO_MSG_STATUS &&
                          frame->type != SC_AUDIO_MSG_PLAYING &&
                          frame->type != SC_AUDIO_MSG_STOPPED &&
                          frame->type != SC_AUDIO_MSG_TIMER_EXPIRED &&
                          frame->type != SC_AUDIO_MSG_ERROR)) {
        return false;
    }
    return decode_status_payload(frame->payload, frame->payload_len, status);
}

bool sc_audio_parse_result(const sc_frame_t *frame,
                           uint8_t *request_type,
                           sc_audio_result_t *result,
                           uint32_t *command_id,
                           sc_audio_status_t *status)
{
    if (frame == NULL || frame->type != SC_AUDIO_MSG_RESULT ||
        frame->payload_len != SC_AUDIO_RESULT_PAYLOAD_SIZE ||
        request_type == NULL || result == NULL || command_id == NULL ||
        status == NULL || frame->payload[1] > SC_AUDIO_RESULT_MALFORMED) {
        return false;
    }
    *request_type = frame->payload[0];
    *result = (sc_audio_result_t)frame->payload[1];
    *command_id = get_u32(&frame->payload[2]);
    return decode_status_payload(&frame->payload[6],
                                 SC_AUDIO_STATUS_PAYLOAD_SIZE, status);
}

bool sc_audio_status_equal(const sc_audio_status_t *left,
                           const sc_audio_status_t *right)
{
    return left != NULL && right != NULL &&
           left->audio_session_id == right->audio_session_id &&
           left->state == right->state &&
           left->volume_percent == right->volume_percent &&
           left->stop_reason == right->stop_reason &&
           left->flags == right->flags &&
           left->remaining_seconds == right->remaining_seconds &&
           left->fault_code == right->fault_code;
}

uint8_t sc_audio_event_type(const sc_audio_status_t *previous,
                            const sc_audio_status_t *current,
                            bool first_report)
{
    if (!valid_status(current)) {
        return SC_AUDIO_MSG_ERROR;
    }
    if (current->state == SC_AUDIO_STATE_FAULT) {
        return SC_AUDIO_MSG_ERROR;
    }
    if ((first_report || (previous != NULL &&
                          previous->state == SC_AUDIO_STATE_NOT_READY)) &&
        current->state != SC_AUDIO_STATE_NOT_READY) {
        return SC_AUDIO_MSG_READY;
    }
    if (previous == NULL || previous->state != current->state) {
        if (current->state == SC_AUDIO_STATE_PLAYING) {
            return SC_AUDIO_MSG_PLAYING;
        }
        if (current->state == SC_AUDIO_STATE_STOPPED) {
            return current->stop_reason == SC_AUDIO_STOP_REASON_TIMER_EXPIRED
                       ? SC_AUDIO_MSG_TIMER_EXPIRED
                       : SC_AUDIO_MSG_STOPPED;
        }
    }
    return SC_AUDIO_MSG_STATUS;
}

bool sc_audio_make_status_event(sc_frame_t *frame,
                                uint8_t event_type,
                                const sc_audio_status_t *status)
{
    if (frame == NULL || !valid_status(status) ||
        (event_type != SC_AUDIO_MSG_READY &&
         event_type != SC_AUDIO_MSG_STATUS &&
         event_type != SC_AUDIO_MSG_PLAYING &&
         event_type != SC_AUDIO_MSG_STOPPED &&
         event_type != SC_AUDIO_MSG_TIMER_EXPIRED &&
         event_type != SC_AUDIO_MSG_ERROR)) {
        return false;
    }
    memset(frame, 0, sizeof(*frame));
    frame->type = event_type;
    frame->sequence = 0U;
    frame->payload_len = SC_AUDIO_STATUS_PAYLOAD_SIZE;
    encode_status_payload(frame->payload, status);
    return true;
}

const char *sc_audio_message_name(uint8_t type)
{
    switch (type) {
    case SC_AUDIO_MSG_HELLO: return "HELLO";
    case SC_AUDIO_MSG_GET_STATUS: return "GET_STATUS";
    case SC_AUDIO_MSG_PLAY: return "PLAY";
    case SC_AUDIO_MSG_STOP: return "STOP";
    case SC_AUDIO_MSG_SET_VOLUME: return "SET_VOLUME";
    case SC_AUDIO_MSG_SET_SOURCE: return "SET_SOURCE";
    case SC_AUDIO_MSG_SET_TONE_NOTE: return "SET_TONE_NOTE";
    case SC_AUDIO_MSG_SET_I2S_PACKING: return "SET_I2S_PACKING";
    case SC_AUDIO_MSG_INFO: return "INFO";
    case SC_AUDIO_MSG_RESULT: return "RESULT";
    case SC_AUDIO_MSG_READY: return "READY";
    case SC_AUDIO_MSG_STATUS: return "STATUS";
    case SC_AUDIO_MSG_PLAYING: return "PLAYING";
    case SC_AUDIO_MSG_STOPPED: return "STOPPED";
    case SC_AUDIO_MSG_TIMER_EXPIRED: return "TIMER_EXPIRED";
    case SC_AUDIO_MSG_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

const char *sc_audio_state_name(sc_audio_state_t state)
{
    switch (state) {
    case SC_AUDIO_STATE_NOT_READY: return "NOT_READY";
    case SC_AUDIO_STATE_STOPPED: return "STOPPED";
    case SC_AUDIO_STATE_STARTING: return "STARTING";
    case SC_AUDIO_STATE_PLAYING: return "PLAYING";
    case SC_AUDIO_STATE_STOPPING: return "STOPPING";
    case SC_AUDIO_STATE_FAULT: return "FAULT";
    default: return "UNKNOWN";
    }
}

const char *sc_audio_result_name(sc_audio_result_t result)
{
    switch (result) {
    case SC_AUDIO_RESULT_ACCEPTED: return "ACCEPTED";
    case SC_AUDIO_RESULT_NO_CHANGE: return "NO_CHANGE";
    case SC_AUDIO_RESULT_DUPLICATE: return "DUPLICATE";
    case SC_AUDIO_RESULT_NOT_READY: return "NOT_READY";
    case SC_AUDIO_RESULT_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case SC_AUDIO_RESULT_SESSION_MISMATCH: return "SESSION_MISMATCH";
    case SC_AUDIO_RESULT_STALE_COMMAND: return "STALE_COMMAND";
    case SC_AUDIO_RESULT_COMMAND_ID_REUSED: return "COMMAND_ID_REUSED";
    case SC_AUDIO_RESULT_BUSY: return "BUSY";
    case SC_AUDIO_RESULT_INTERNAL_ERROR: return "INTERNAL_ERROR";
    case SC_AUDIO_RESULT_UNSUPPORTED: return "UNSUPPORTED";
    case SC_AUDIO_RESULT_MALFORMED: return "MALFORMED";
    default: return "UNKNOWN";
    }
}
