#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sc_audio_protocol.h"

typedef struct {
    sc_audio_status_t status;
    unsigned play_calls;
    unsigned stop_calls;
    unsigned volume_calls;
    uint32_t last_duration;
} fake_backend_t;

static sc_audio_result_t get_status(void *context, sc_audio_status_t *status)
{
    fake_backend_t *fake = context;
    *status = fake->status;
    return SC_AUDIO_RESULT_ACCEPTED;
}

static sc_audio_result_t play(void *context, uint32_t duration_seconds)
{
    fake_backend_t *fake = context;
    if (fake->status.state != SC_AUDIO_STATE_STOPPED) {
        return SC_AUDIO_RESULT_BUSY;
    }
    ++fake->play_calls;
    fake->last_duration = duration_seconds;
    fake->status.state = SC_AUDIO_STATE_STARTING;
    fake->status.remaining_seconds = duration_seconds;
    return SC_AUDIO_RESULT_ACCEPTED;
}

static sc_audio_result_t stop(void *context)
{
    fake_backend_t *fake = context;
    if (fake->status.state == SC_AUDIO_STATE_STOPPED) {
        return SC_AUDIO_RESULT_NO_CHANGE;
    }
    ++fake->stop_calls;
    fake->status.state = SC_AUDIO_STATE_STOPPING;
    return SC_AUDIO_RESULT_ACCEPTED;
}

static sc_audio_result_t set_volume(void *context, uint8_t volume_percent)
{
    fake_backend_t *fake = context;
    ++fake->volume_calls;
    fake->status.volume_percent = volume_percent;
    return SC_AUDIO_RESULT_ACCEPTED;
}

static void expect_result(const sc_frame_t *response,
                          uint8_t expected_request,
                          sc_audio_result_t expected_result,
                          uint32_t expected_command)
{
    uint8_t request_type = 0;
    sc_audio_result_t result = SC_AUDIO_RESULT_INTERNAL_ERROR;
    uint32_t command_id = 0;
    sc_audio_status_t status = {0};
    assert(sc_audio_parse_result(response, &request_type, &result,
                                 &command_id, &status));
    assert(request_type == expected_request);
    assert(result == expected_result);
    assert(command_id == expected_command);
}

static void handshake_and_status(void)
{
    fake_backend_t fake = {
        .status = {
            .audio_session_id = 0x11223344U,
            .state = SC_AUDIO_STATE_STOPPED,
            .volume_percent = 50U,
            .stop_reason = SC_AUDIO_STOP_REASON_BOOT,
        },
    };
    const sc_audio_backend_t backend = {
        .context = &fake,
        .get_status = get_status,
        .play = play,
        .stop = stop,
        .set_volume = set_volume,
    };
    sc_audio_server_t server;
    sc_audio_server_init(&server, 0x11223344U, 1U, 3600U, &backend);

    sc_frame_t request;
    sc_frame_t response;
    assert(sc_audio_make_hello(&request, 7U, 0xaabbccddU));
    assert(sc_audio_server_handle(&server, &request, &response));
    assert(response.sequence == 7U);
    sc_audio_info_t info = {0};
    assert(sc_audio_parse_info(&response, &info));
    assert(info.control_session_id == 0xaabbccddU);
    assert(info.audio_session_id == 0x11223344U);
    assert(info.semantic_version == SC_AUDIO_SEMANTIC_VERSION);
    assert(info.minimum_duration_seconds == 1U);
    assert(info.maximum_duration_seconds == 3600U);

    assert(sc_audio_make_get_status(&request, 8U));
    assert(sc_audio_server_handle(&server, &request, &response));
    sc_audio_status_t status = {0};
    assert(sc_audio_parse_status(&response, &status));
    assert(status.state == SC_AUDIO_STATE_STOPPED);
    assert(status.volume_percent == 50U);
}

static void commands_are_bounded_and_idempotent(void)
{
    fake_backend_t fake = {
        .status = {
            .audio_session_id = 0x1234U,
            .state = SC_AUDIO_STATE_STOPPED,
            .volume_percent = 25U,
            .stop_reason = SC_AUDIO_STOP_REASON_BOOT,
        },
    };
    const sc_audio_backend_t backend = {
        .context = &fake,
        .get_status = get_status,
        .play = play,
        .stop = stop,
        .set_volume = set_volume,
    };
    sc_audio_server_t server;
    sc_audio_server_init(&server, 0x1234U, 1U, 3600U, &backend);
    sc_frame_t request;
    sc_frame_t response;
    assert(sc_audio_make_hello(&request, 1U, 0x5678U));
    assert(sc_audio_server_handle(&server, &request, &response));

    assert(sc_audio_make_play(&request, 2U, 0x5678U, 0x1234U, 1U, 30U));
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_PLAY, SC_AUDIO_RESULT_ACCEPTED, 1U);
    assert(fake.play_calls == 1U && fake.last_duration == 30U);

    request.sequence = 3U;
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_PLAY, SC_AUDIO_RESULT_DUPLICATE, 1U);
    assert(fake.play_calls == 1U);

    assert(sc_audio_make_stop(&request, 4U, 0x5678U, 0x1234U, 1U));
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_STOP,
                  SC_AUDIO_RESULT_COMMAND_ID_REUSED, 1U);
    assert(fake.stop_calls == 0U);

    assert(sc_audio_make_stop(&request, 5U, 0x5678U, 0x1234U, 2U));
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_STOP, SC_AUDIO_RESULT_ACCEPTED, 2U);
    assert(fake.stop_calls == 1U);

    assert(sc_audio_make_set_volume(&request, 6U, 0x5678U, 0x1234U, 1U, 75U));
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_SET_VOLUME,
                  SC_AUDIO_RESULT_STALE_COMMAND, 1U);
    assert(fake.volume_calls == 0U);

    assert(sc_audio_make_set_volume(&request, 7U, 0x5678U, 0x1234U, 3U, 101U));
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_SET_VOLUME,
                  SC_AUDIO_RESULT_INVALID_ARGUMENT, 3U);
    assert(fake.volume_calls == 0U);

    assert(sc_audio_make_play(&request, 8U, 0x5678U, 0x1234U, 4U, 0U));
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_PLAY,
                  SC_AUDIO_RESULT_INVALID_ARGUMENT, 4U);
    assert(fake.play_calls == 1U);

    assert(sc_audio_make_play(&request, 9U, 0x5678U, 0x9999U, 5U, 30U));
    assert(sc_audio_server_handle(&server, &request, &response));
    expect_result(&response, SC_AUDIO_MSG_PLAY,
                  SC_AUDIO_RESULT_SESSION_MISMATCH, 5U);
    assert(fake.play_calls == 1U);
}

static void reset_reconciliation_and_events(void)
{
    fake_backend_t fake = {
        .status = {
            .audio_session_id = 77U,
            .state = SC_AUDIO_STATE_PLAYING,
            .volume_percent = 75U,
            .stop_reason = SC_AUDIO_STOP_REASON_BOOT,
            .remaining_seconds = 123U,
        },
    };
    const sc_audio_backend_t backend = {
        .context = &fake,
        .get_status = get_status,
        .play = play,
        .stop = stop,
        .set_volume = set_volume,
    };
    sc_audio_server_t server;
    sc_audio_server_init(&server, 77U, 1U, 3600U, &backend);
    sc_frame_t request;
    sc_frame_t response;

    /* A newly booted Control establishes a new session without changing audio. */
    assert(sc_audio_make_hello(&request, 1U, 88U));
    assert(sc_audio_server_handle(&server, &request, &response));
    assert(fake.status.state == SC_AUDIO_STATE_PLAYING);
    assert(sc_audio_make_get_status(&request, 2U));
    assert(sc_audio_server_handle(&server, &request, &response));
    sc_audio_status_t parsed = {0};
    assert(sc_audio_parse_status(&response, &parsed));
    assert(parsed.state == SC_AUDIO_STATE_PLAYING);
    assert(parsed.remaining_seconds == 123U);

    sc_frame_t event;
    assert(sc_audio_event_type(NULL, &fake.status, true) == SC_AUDIO_MSG_READY);
    assert(sc_audio_make_status_event(&event, SC_AUDIO_MSG_READY, &fake.status));
    assert(event.sequence == 0U);
    assert(sc_audio_parse_status(&event, &parsed));

    const sc_audio_status_t previous = fake.status;
    fake.status.state = SC_AUDIO_STATE_STOPPED;
    fake.status.stop_reason = SC_AUDIO_STOP_REASON_TIMER_EXPIRED;
    fake.status.remaining_seconds = 0U;
    assert(sc_audio_event_type(&previous, &fake.status, false) ==
           SC_AUDIO_MSG_TIMER_EXPIRED);
}

static void malformed_and_unsupported_fail_safe(void)
{
    fake_backend_t fake = {
        .status = {
            .audio_session_id = 9U,
            .state = SC_AUDIO_STATE_STOPPED,
            .volume_percent = 50U,
            .stop_reason = SC_AUDIO_STOP_REASON_BOOT,
        },
    };
    const sc_audio_backend_t backend = {.context = &fake, .get_status = get_status};
    sc_audio_server_t server;
    sc_audio_server_init(&server, 9U, 1U, 3600U, &backend);
    sc_frame_t request = {.type = SC_AUDIO_MSG_PLAY, .sequence = 10U};
    sc_frame_t response;
    assert(sc_audio_server_handle(&server, &request, &response));
    assert(response.type == SC_AUDIO_MSG_ERROR);
    assert(response.sequence == 10U);
    assert(response.payload[1] == SC_AUDIO_RESULT_MALFORMED);
    assert(fake.play_calls == 0U);

    request.type = 0x55U;
    request.payload_len = 0U;
    assert(sc_audio_server_handle(&server, &request, &response));
    assert(response.type == SC_AUDIO_MSG_ERROR);
    assert(response.payload[1] == SC_AUDIO_RESULT_UNSUPPORTED);
}

int main(void)
{
    handshake_and_status();
    commands_are_bounded_and_idempotent();
    reset_reconciliation_and_events();
    malformed_and_unsupported_fail_safe();
    puts("sc_audio_protocol tests passed");
    return 0;
}
