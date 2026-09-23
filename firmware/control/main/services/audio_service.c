#include "audio_service.h"

#include <stdbool.h>
#include <stdint.h>

#include "sdkconfig.h"
#include "app_core.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if CONFIG_SC_ENABLE_AUDIO
#include "audio_diag_console.h"
#include "audio_player.h"
#include "settings_store.h"
#elif CONFIG_SC_AUDIO_REMOTE_LINK
#include "sc_audio_link.h"
#include "sc_audio_protocol.h"
#endif

static const char *TAG = "sc_audio_service";

#define SC_AUDIO_VOLUME_STEP_PCT (5)
#define SC_AUDIO_DEFAULT_VOLUME_PCT (40U)
#define SC_AUDIO_SETTINGS_KEY_VOLUME "volume_pct"

#if CONFIG_SC_ENABLE_AUDIO || CONFIG_SC_AUDIO_REMOTE_LINK
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
#endif

#if CONFIG_SC_ENABLE_AUDIO

esp_err_t sc_audio_service_start(void)
{
    uint8_t initial_volume = SC_AUDIO_DEFAULT_VOLUME_PCT;
    esp_err_t load_err = sc_settings_store_load_u8(SC_AUDIO_SETTINGS_KEY_VOLUME, SC_AUDIO_DEFAULT_VOLUME_PCT, &initial_volume);
    if (load_err != ESP_OK) {
        ESP_LOGW(TAG, "volume load failed, using default: %s", esp_err_to_name(load_err));
        initial_volume = SC_AUDIO_DEFAULT_VOLUME_PCT;
    }

    ESP_LOGI(TAG, "starting legacy local audio playback service");
    ESP_RETURN_ON_ERROR(sc_audio_player_start(), TAG, "audio player start failed");
    sc_audio_player_set_enabled(false);
    sc_audio_player_set_volume_percent(initial_volume);
#if CONFIG_SC_AUDIO_DIAG_SERIAL_CONSOLE
    ESP_RETURN_ON_ERROR(sc_audio_diag_console_start(), TAG, "audio diagnostic console start failed");
#endif
    ESP_LOGI(TAG, "local audio ready: enabled=%d volume=%u%%",
             sc_audio_player_get_enabled(), (unsigned)sc_audio_player_get_volume_percent());
    return ESP_OK;
}

esp_err_t sc_audio_service_set_playback(bool enable)
{
    const bool current = sc_audio_player_get_enabled();
    if (current == enable) {
        return ESP_OK;
    }

    if (enable) {
        sc_audio_player_set_enabled(true);
    } else {
        sc_audio_player_request_stop();
    }
    post_audio_event(SC_APP_EVT_AUDIO_STATE_CHANGED, enable ? 1 : 0);
    ESP_LOGI(TAG, "local playback %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t sc_audio_service_toggle_playback(void)
{
    return sc_audio_service_set_playback(!sc_audio_player_get_enabled());
}

esp_err_t sc_audio_service_change_volume(int delta_steps)
{
    int next = (int)sc_audio_player_get_volume_percent() +
               (delta_steps * SC_AUDIO_VOLUME_STEP_PCT);
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
    if (sc_audio_player_get_volume_percent() == next) {
        return ESP_OK;
    }

    sc_audio_player_set_volume_percent(next);
    const esp_err_t save_err = sc_settings_store_save_u8(SC_AUDIO_SETTINGS_KEY_VOLUME, next);
    if (save_err != ESP_OK) {
        ESP_LOGW(TAG, "volume save failed: %s", esp_err_to_name(save_err));
    }
    post_audio_event(SC_APP_EVT_AUDIO_VOLUME_CHANGED, next);
    ESP_LOGI(TAG, "local volume=%u%%", (unsigned)next);
    return ESP_OK;
}

bool sc_audio_service_get_playback_enabled(void)
{
    return sc_audio_player_get_enabled();
}

uint8_t sc_audio_service_get_volume_percent(void)
{
    return sc_audio_player_get_volume_percent();
}

bool sc_audio_service_is_available(void)
{
    return true;
}

bool sc_audio_service_is_synchronized(void)
{
    return true;
}

#elif CONFIG_SC_AUDIO_REMOTE_LINK

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

static bool remote_state_available(sc_audio_state_t state)
{
    return state == SC_AUDIO_STATE_STOPPED ||
           state == SC_AUDIO_STATE_STARTING ||
           state == SC_AUDIO_STATE_PLAYING ||
           state == SC_AUDIO_STATE_STOPPING;
}

static bool remote_state_playback_active(sc_audio_state_t state)
{
    return state == SC_AUDIO_STATE_STARTING ||
           state == SC_AUDIO_STATE_PLAYING ||
           state == SC_AUDIO_STATE_STOPPING;
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
    uint8_t volume = 0U;
    bool playback_settle_timeout = false;
    bool volume_settle_timeout = false;

    portENTER_CRITICAL(&s_remote_lock);
    const bool was_synchronized = s_remote.synchronized;
    const bool was_available = was_synchronized &&
                               remote_state_available(s_remote.status.state);
    const bool was_active = was_synchronized &&
                            remote_state_playback_active(s_remote.status.state);
    const uint8_t previous_volume = s_remote.status.volume_percent;

    if (!synchronized) {
        s_remote.synchronized = false;
        s_remote.playback_request_pending = false;
        s_remote.volume_request_pending = false;
        s_remote.volume_dirty = false;
    } else {
        s_remote.synchronized = true;
        s_remote.status = status;
        available = remote_state_available(status.state);
        playback_active = remote_state_playback_active(status.state);
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
                                       : (status.state == SC_AUDIO_STATE_STOPPING ||
                                          status.state == SC_AUDIO_STATE_STOPPED);
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
        playback_active = remote_state_playback_active(s_remote.status.state);
        notify_playback = (was_synchronized && was_active != playback_active) ||
                          (!was_synchronized && playback_active);
        volume = s_remote.status.volume_percent;
        notify_volume = !was_synchronized || previous_volume != volume;
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
        vTaskDelay(pdMS_TO_TICKS(SC_REMOTE_POLL_MS));
    }
}

esp_err_t sc_audio_service_start(void)
{
    portENTER_CRITICAL(&s_remote_lock);
    s_remote = (sc_remote_audio_state_t){0};
    s_remote.status.state = SC_AUDIO_STATE_NOT_READY;
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

    portENTER_CRITICAL(&s_remote_lock);
    synchronized = s_remote.synchronized;
    available = synchronized && remote_state_available(s_remote.status.state);
    current = synchronized && remote_state_playback_active(s_remote.status.state);
    pending = s_remote.playback_request_pending;
    if (synchronized && available && !pending && current != enable) {
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

    const esp_err_t error = enable
                                ? sc_audio_link_control_play(
                                      SC_AUDIO_LINK_DEFAULT_PLAY_SECONDS)
                                : sc_audio_link_control_stop();
    if (error != ESP_OK) {
        portENTER_CRITICAL(&s_remote_lock);
        s_remote.playback_request_pending = false;
        portEXIT_CRITICAL(&s_remote_lock);
        return error;
    }
    ESP_LOGI(TAG, "remote %s requested",
             enable ? "PLAY(1800s)" : "STOP");
    return ESP_OK;
}

esp_err_t sc_audio_service_toggle_playback(void)
{
    bool synchronized;
    bool available;
    bool active;
    bool pending;

    portENTER_CRITICAL(&s_remote_lock);
    synchronized = s_remote.synchronized;
    available = synchronized && remote_state_available(s_remote.status.state);
    active = synchronized && remote_state_playback_active(s_remote.status.state);
    pending = s_remote.playback_request_pending;
    portEXIT_CRITICAL(&s_remote_lock);

    if (!synchronized || !available || pending) {
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
              remote_state_playback_active(s_remote.status.state);
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

#else

esp_err_t sc_audio_service_start(void)
{
    ESP_LOGI(TAG, "audio service disabled by config");
    return ESP_OK;
}

esp_err_t sc_audio_service_set_playback(bool enable)
{
    (void)enable;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_audio_service_toggle_playback(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_audio_service_change_volume(int delta_steps)
{
    (void)delta_steps;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_audio_service_set_volume_percent(uint8_t percent)
{
    (void)percent;
    return ESP_ERR_NOT_SUPPORTED;
}

bool sc_audio_service_get_playback_enabled(void)
{
    return false;
}

uint8_t sc_audio_service_get_volume_percent(void)
{
    return 0U;
}

bool sc_audio_service_is_available(void)
{
    return false;
}

bool sc_audio_service_is_synchronized(void)
{
    return false;
}

#endif
