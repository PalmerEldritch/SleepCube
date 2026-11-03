#include "audio_service.h"

#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "app_core.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sc_audio_link.h"
#include "sc_audio_protocol.h"

static const char *TAG = "sc_audio_service";

#define SC_AUDIO_VOLUME_STEP_PCT (5)
static void post_audio_event(sc_app_event_type_t type, int32_t value)
{
    const sc_app_event_t event = {
        .type = type,
        .value = value,
    };
    const esp_err_t error = sc_app_core_post_event(&event);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "audio event %d dropped: %s", (int)type,
                 esp_err_to_name(error));
    }
}

#define SC_REMOTE_POLL_MS (20U)
#define SC_REMOTE_COMMAND_SETTLE_MS (1500U)

typedef struct {
    bool synchronized;
    sc_audio_status_t status;
    bool playback_request_pending;
    bool requested_playback;
    TickType_t playback_deadline;
    bool volume_dirty;
    bool volume_request_pending;
    uint8_t desired_volume;
    uint8_t sent_volume;
    TickType_t volume_deadline;
} sc_remote_audio_state_t;

static portMUX_TYPE s_remote_lock = portMUX_INITIALIZER_UNLOCKED;
static sc_remote_audio_state_t s_remote;
static volatile bool s_restore_debug_defaults_pending;

static bool remote_state_available(sc_audio_state_t state)
{
    return state == SC_AUDIO_STATE_STOPPED ||
           state == SC_AUDIO_STATE_STARTING ||
           state == SC_AUDIO_STATE_PLAYING ||
           state == SC_AUDIO_STATE_STOPPING;
}

static bool remote_state_presented_playing(sc_audio_state_t state)
{
    return state == SC_AUDIO_STATE_STARTING ||
           state == SC_AUDIO_STATE_PLAYING;
}

static bool deadline_reached(TickType_t now, TickType_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

static void remote_update_snapshot(void)
{
    sc_audio_status_t status = {0};
    const bool synchronized = sc_audio_link_control_is_synchronized() &&
                              sc_audio_link_control_get_status(&status) == ESP_OK;
    const TickType_t now = xTaskGetTickCount();
    bool notify_availability = false;
    bool available = false;
    bool notify_playback = false;
    bool playback_active = false;
    bool notify_volume = false;
    bool notify_debug_config = false;
    uint8_t volume = 0U;
    bool playback_settle_timeout = false;
    bool volume_settle_timeout = false;

    portENTER_CRITICAL(&s_remote_lock);
    const bool was_synchronized = s_remote.synchronized;
    const bool was_available = was_synchronized &&
                               remote_state_available(s_remote.status.state);
    const bool was_active = was_synchronized &&
                            remote_state_presented_playing(s_remote.status.state);
    const uint8_t previous_volume = s_remote.status.volume_percent;
    const uint8_t previous_flags = s_remote.status.flags;

    if (!synchronized) {
        s_remote.synchronized = false;
        s_remote.playback_request_pending = false;
        s_remote.volume_request_pending = false;
        s_remote.volume_dirty = false;
    } else {
        s_remote.synchronized = true;
        s_remote.status = status;
        available = remote_state_available(status.state);
        playback_active = remote_state_presented_playing(status.state);
        volume = status.volume_percent;

        if (!was_synchronized) {
            s_remote.desired_volume = status.volume_percent;
            s_remote.volume_dirty = false;
            s_remote.volume_request_pending = false;
            s_remote.playback_request_pending = false;
        }

        if (s_remote.playback_request_pending) {
            const bool confirmed = s_remote.requested_playback
                                       ? (status.state == SC_AUDIO_STATE_STARTING ||
                                          status.state == SC_AUDIO_STATE_PLAYING)
                                       : status.state == SC_AUDIO_STATE_STOPPED;
            if (confirmed) {
                s_remote.playback_request_pending = false;
            } else if (deadline_reached(now, s_remote.playback_deadline)) {
                s_remote.playback_request_pending = false;
                playback_settle_timeout = true;
            }
        }

        if (s_remote.volume_request_pending) {
            if (status.volume_percent == s_remote.sent_volume) {
                s_remote.volume_request_pending = false;
                s_remote.volume_dirty = s_remote.desired_volume != status.volume_percent;
            } else if (deadline_reached(now, s_remote.volume_deadline)) {
                s_remote.volume_request_pending = false;
                s_remote.volume_dirty = false;
                s_remote.desired_volume = status.volume_percent;
                volume_settle_timeout = true;
            }
        }
    }

    notify_availability = was_available !=
                          (s_remote.synchronized &&
                           remote_state_available(s_remote.status.state));
    available = s_remote.synchronized &&
                remote_state_available(s_remote.status.state);
    if (s_remote.synchronized) {
        playback_active = remote_state_presented_playing(s_remote.status.state);
        notify_playback = (was_synchronized && was_active != playback_active) ||
                          (!was_synchronized && playback_active);
        volume = s_remote.status.volume_percent;
        notify_volume = !was_synchronized || previous_volume != volume;
        notify_debug_config = !was_synchronized || previous_flags != s_remote.status.flags;
    }
    portEXIT_CRITICAL(&s_remote_lock);

    if (playback_settle_timeout) {
        ESP_LOGW(TAG, "remote playback request did not reach confirmed state");
    }
    if (volume_settle_timeout) {
        ESP_LOGW(TAG, "remote volume request did not reach confirmed state");
    }
    if (notify_availability) {
        post_audio_event(SC_APP_EVT_AUDIO_AVAILABILITY_CHANGED,
                         available ? 1 : 0);
    }
    if (notify_playback) {
        post_audio_event(SC_APP_EVT_AUDIO_STATE_CHANGED,
                         playback_active ? 1 : 0);
    }
    if (notify_volume) {
        post_audio_event(SC_APP_EVT_AUDIO_VOLUME_CHANGED, volume);
    }
    if (notify_debug_config) {
        post_audio_event(SC_APP_EVT_AUDIO_DEBUG_CONFIG_CHANGED,
                         s_remote.status.flags);
    }
}

static void remote_send_coalesced_volume(void)
{
    uint8_t target = 0U;
    bool should_send = false;

    portENTER_CRITICAL(&s_remote_lock);
    if (s_remote.synchronized &&
        remote_state_available(s_remote.status.state) &&
        s_remote.volume_dirty && !s_remote.volume_request_pending) {
        target = s_remote.desired_volume;
        if (target == s_remote.status.volume_percent) {
            s_remote.volume_dirty = false;
        } else {
            s_remote.volume_request_pending = true;
            s_remote.sent_volume = target;
            s_remote.volume_deadline = xTaskGetTickCount() +
                pdMS_TO_TICKS(SC_REMOTE_COMMAND_SETTLE_MS);
            should_send = true;
        }
    }
    portEXIT_CRITICAL(&s_remote_lock);

    if (!should_send) {
        return;
    }

    const esp_err_t error = sc_audio_link_control_set_volume(target);
    if (error != ESP_OK) {
        portENTER_CRITICAL(&s_remote_lock);
        s_remote.volume_request_pending = false;
        s_remote.volume_dirty = false;
        s_remote.desired_volume = s_remote.status.volume_percent;
        portEXIT_CRITICAL(&s_remote_lock);
        ESP_LOGW(TAG, "remote volume request rejected: %s",
                 esp_err_to_name(error));
    } else {
        ESP_LOGI(TAG, "remote volume=%u%% requested", (unsigned)target);
    }
}

static void remote_audio_task(void *argument)
{
    (void)argument;
    for (;;) {
        remote_update_snapshot();
        remote_send_coalesced_volume();
        if (s_restore_debug_defaults_pending &&
            sc_audio_link_control_has_capability(SC_AUDIO_CAP_DEBUG_SOURCE)) {
            const esp_err_t note_error =
                sc_audio_link_control_set_tone_note(SC_AUDIO_TONE_C5);
            const esp_err_t packing_error =
                sc_audio_link_control_set_i2s_packing(
                    SC_AUDIO_I2S_PACKING_NATIVE);
            const esp_err_t source_error =
                sc_audio_link_control_set_source(SC_AUDIO_SOURCE_MUSIC);
            if (note_error == ESP_OK && packing_error == ESP_OK &&
                source_error == ESP_OK) {
                s_restore_debug_defaults_pending = false;
                ESP_LOGI(TAG,
                         "debug audio defaults queued: music/C5/native slots");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SC_REMOTE_POLL_MS));
    }
}

esp_err_t sc_audio_service_start(void)
{
    portENTER_CRITICAL(&s_remote_lock);
    s_remote = (sc_remote_audio_state_t){0};
    s_remote.status.state = SC_AUDIO_STATE_NOT_READY;
    s_restore_debug_defaults_pending = false;
    portEXIT_CRITICAL(&s_remote_lock);

    if (xTaskCreate(remote_audio_task, "audio_facade", 4096, NULL, 4, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "remote semantic audio facade started");
    return ESP_OK;
}

esp_err_t sc_audio_service_set_playback(bool enable)
{
    bool synchronized;
    bool available;
    bool current;
    bool pending;
    bool stopping;

    portENTER_CRITICAL(&s_remote_lock);
    synchronized = s_remote.synchronized;
    available = synchronized && remote_state_available(s_remote.status.state);
    current = synchronized &&
              remote_state_presented_playing(s_remote.status.state);
    pending = s_remote.playback_request_pending;
    stopping = synchronized && s_remote.status.state == SC_AUDIO_STATE_STOPPING;
    if (synchronized && available && !pending && !stopping &&
        current != enable) {
        s_remote.playback_request_pending = true;
        s_remote.requested_playback = enable;
        s_remote.playback_deadline = xTaskGetTickCount() +
            pdMS_TO_TICKS(SC_REMOTE_COMMAND_SETTLE_MS);
    }
    portEXIT_CRITICAL(&s_remote_lock);

    if (!synchronized || !available || pending) {
        return ESP_ERR_INVALID_STATE;
    }
    if (current == enable) {
        return ESP_OK;
    }
    if (stopping) {
        return ESP_ERR_INVALID_STATE;
    }

    const esp_err_t error = enable
                                ? sc_audio_link_control_play(
                                      CONFIG_SC_REMOTE_PLAY_DURATION_SECONDS)
                                : sc_audio_link_control_stop();
    if (error != ESP_OK) {
        portENTER_CRITICAL(&s_remote_lock);
        s_remote.playback_request_pending = false;
        portEXIT_CRITICAL(&s_remote_lock);
        return error;
    }
    if (enable) {
        ESP_LOGI(TAG, "remote PLAY(%ds) requested",
                 CONFIG_SC_REMOTE_PLAY_DURATION_SECONDS);
    } else {
        ESP_LOGI(TAG, "remote STOP requested");
    }
    return ESP_OK;
}

esp_err_t sc_audio_service_toggle_playback(void)
{
    bool synchronized;
    bool available;
    bool active;
    bool pending;
    bool stopping;

    portENTER_CRITICAL(&s_remote_lock);
    synchronized = s_remote.synchronized;
    available = synchronized && remote_state_available(s_remote.status.state);
    active = synchronized &&
             remote_state_presented_playing(s_remote.status.state);
    pending = s_remote.playback_request_pending;
    stopping = synchronized && s_remote.status.state == SC_AUDIO_STATE_STOPPING;
    portEXIT_CRITICAL(&s_remote_lock);

    if (!synchronized || !available || pending || stopping) {
        return ESP_ERR_INVALID_STATE;
    }
    return sc_audio_service_set_playback(!active);
}

esp_err_t sc_audio_service_change_volume(int delta_steps)
{
    int next;
    bool synchronized;
    bool available;

    portENTER_CRITICAL(&s_remote_lock);
    synchronized = s_remote.synchronized;
    available = synchronized && remote_state_available(s_remote.status.state);
    next = (int)(s_remote.volume_dirty || s_remote.volume_request_pending
                     ? s_remote.desired_volume
                     : s_remote.status.volume_percent) +
           (delta_steps * SC_AUDIO_VOLUME_STEP_PCT);
    portEXIT_CRITICAL(&s_remote_lock);

    if (!synchronized || !available) {
        return ESP_ERR_INVALID_STATE;
    }
    if (next < 0) {
        next = 0;
    }
    if (next > 100) {
        next = 100;
    }
    return sc_audio_service_set_volume_percent((uint8_t)next);
}

esp_err_t sc_audio_service_set_volume_percent(uint8_t percent)
{
    const uint8_t next = percent > 100U ? 100U : percent;
    bool synchronized;
    bool available;

    portENTER_CRITICAL(&s_remote_lock);
    synchronized = s_remote.synchronized;
    available = synchronized && remote_state_available(s_remote.status.state);
    if (available) {
        s_remote.desired_volume = next;
        s_remote.volume_dirty = s_remote.status.volume_percent != next;
    }
    portEXIT_CRITICAL(&s_remote_lock);

    return synchronized && available ? ESP_OK : ESP_ERR_INVALID_STATE;
}

bool sc_audio_service_get_playback_enabled(void)
{
    bool enabled;
    portENTER_CRITICAL(&s_remote_lock);
    enabled = s_remote.synchronized &&
              remote_state_presented_playing(s_remote.status.state);
    portEXIT_CRITICAL(&s_remote_lock);
    return enabled;
}

uint8_t sc_audio_service_get_volume_percent(void)
{
    uint8_t volume;
    portENTER_CRITICAL(&s_remote_lock);
    volume = s_remote.status.volume_percent;
    portEXIT_CRITICAL(&s_remote_lock);
    return volume;
}

bool sc_audio_service_is_available(void)
{
    bool available;
    portENTER_CRITICAL(&s_remote_lock);
    available = s_remote.synchronized &&
                remote_state_available(s_remote.status.state);
    portEXIT_CRITICAL(&s_remote_lock);
    return available;
}

bool sc_audio_service_is_synchronized(void)
{
    bool synchronized;
    portENTER_CRITICAL(&s_remote_lock);
    synchronized = s_remote.synchronized;
    portEXIT_CRITICAL(&s_remote_lock);
    return synchronized;
}

bool sc_audio_service_has_debug_source(void)
{
    return sc_audio_link_control_has_capability(SC_AUDIO_CAP_DEBUG_SOURCE);
}

static void remote_get_debug_config(sc_audio_source_t *source,
                                    sc_audio_tone_note_t *note,
                                    sc_audio_i2s_packing_t *packing)
{
    sc_audio_status_t status;
    portENTER_CRITICAL(&s_remote_lock);
    status = s_remote.status;
    portEXIT_CRITICAL(&s_remote_lock);
    if (!sc_audio_debug_config_from_status(&status, source, note, packing)) {
        *source = SC_AUDIO_SOURCE_MUSIC;
        *note = SC_AUDIO_TONE_C5;
        *packing = SC_AUDIO_I2S_PACKING_NATIVE;
    }
}

sc_audio_source_t sc_audio_service_get_source(void)
{
    sc_audio_source_t source;
    sc_audio_tone_note_t note;
    sc_audio_i2s_packing_t packing;
    remote_get_debug_config(&source, &note, &packing);
    return source;
}

sc_audio_tone_note_t sc_audio_service_get_tone_note(void)
{
    sc_audio_source_t source;
    sc_audio_tone_note_t note;
    sc_audio_i2s_packing_t packing;
    remote_get_debug_config(&source, &note, &packing);
    return note;
}

sc_audio_i2s_packing_t sc_audio_service_get_i2s_packing(void)
{
    sc_audio_source_t source;
    sc_audio_tone_note_t note;
    sc_audio_i2s_packing_t packing;
    remote_get_debug_config(&source, &note, &packing);
    return packing;
}

esp_err_t sc_audio_service_set_source(sc_audio_source_t source)
{
    return sc_audio_link_control_set_source(source);
}

esp_err_t sc_audio_service_set_tone_note(sc_audio_tone_note_t note)
{
    return sc_audio_link_control_set_tone_note(note);
}

esp_err_t sc_audio_service_set_i2s_packing(sc_audio_i2s_packing_t packing)
{
    return sc_audio_link_control_set_i2s_packing(packing);
}

void sc_audio_service_restore_debug_defaults(void)
{
    s_restore_debug_defaults_pending = true;
}
