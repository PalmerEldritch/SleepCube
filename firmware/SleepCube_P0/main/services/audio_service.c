#include "audio_service.h"

#include <stdbool.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_log.h"

#if CONFIG_SC_ENABLE_AUDIO
#include "audio_diag_console.h"
#include "audio_player.h"
#include "light_service.h"
#include "settings_store.h"
#endif

static const char *TAG = "sc_audio_service";

#define SC_AUDIO_VOLUME_STEP_PCT (5)
#define SC_AUDIO_DEFAULT_VOLUME_PCT (40U)
#define SC_AUDIO_SETTINGS_KEY_VOLUME "volume_pct"

#if CONFIG_SC_ENABLE_AUDIO

esp_err_t sc_audio_service_start(void)
{
    uint8_t initial_volume = SC_AUDIO_DEFAULT_VOLUME_PCT;
    esp_err_t load_err = sc_settings_store_load_u8(SC_AUDIO_SETTINGS_KEY_VOLUME, SC_AUDIO_DEFAULT_VOLUME_PCT, &initial_volume);
    if (load_err != ESP_OK) {
        ESP_LOGW(TAG, "volume load failed, using default: %s", esp_err_to_name(load_err));
        initial_volume = SC_AUDIO_DEFAULT_VOLUME_PCT;
    }

    ESP_LOGI(TAG, "starting audio playback service");
    ESP_RETURN_ON_ERROR(sc_audio_player_start(), TAG, "audio player start failed");
    sc_audio_player_set_enabled(false);
    sc_audio_player_set_volume_percent(initial_volume);
#if CONFIG_SC_AUDIO_DIAG_SERIAL_CONSOLE
    ESP_RETURN_ON_ERROR(sc_audio_diag_console_start(), TAG, "audio diagnostic console start failed");
#endif
    ESP_LOGI(TAG, "audio control ready: enabled=%d volume=%u%%",
             sc_audio_player_get_enabled(), (unsigned)sc_audio_player_get_volume_percent());
    return ESP_OK;
}

esp_err_t sc_audio_service_set_playback(bool enable)
{
    bool current = sc_audio_player_get_enabled();
    if (current == enable) {
        ESP_LOGI(TAG, "playback already %s", enable ? "enabled" : "disabled");
        return ESP_OK;
    }

    if (enable) {
        sc_audio_player_set_enabled(true);
    } else {
        sc_audio_player_request_stop();
    }
    esp_err_t sway_err = sc_light_service_audio_sway(enable);
    if (sway_err != ESP_OK && sway_err != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "light audio sway failed: %s", esp_err_to_name(sway_err));
    }
    ESP_LOGI(TAG, "playback %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t sc_audio_service_toggle_playback(void)
{
    bool next = !sc_audio_player_get_enabled();
    return sc_audio_service_set_playback(next);
}

esp_err_t sc_audio_service_change_volume(int delta_steps)
{
    int next = (int)sc_audio_player_get_volume_percent() + (delta_steps * SC_AUDIO_VOLUME_STEP_PCT);
    if (next < 0) {
        next = 0;
    }
    if (next > 100) {
        next = 100;
    }
    sc_audio_player_set_volume_percent((uint8_t)next);
    esp_err_t save_err = sc_settings_store_save_u8(SC_AUDIO_SETTINGS_KEY_VOLUME, (uint8_t)next);
    if (save_err != ESP_OK) {
        ESP_LOGW(TAG, "volume save failed: %s", esp_err_to_name(save_err));
    }
    ESP_LOGI(TAG, "volume=%d%%", next);
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

#else

esp_err_t sc_audio_service_start(void)
{
    ESP_LOGI(TAG, "audio service disabled by config");
    return ESP_OK;
}

esp_err_t sc_audio_service_set_playback(bool enable)
{
    (void)enable;
    ESP_LOGW(TAG, "set playback ignored: audio service disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_audio_service_toggle_playback(void)
{
    ESP_LOGW(TAG, "toggle playback ignored: audio service disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_audio_service_change_volume(int delta_steps)
{
    (void)delta_steps;
    ESP_LOGW(TAG, "change volume ignored: audio service disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

bool sc_audio_service_get_playback_enabled(void)
{
    return false;
}

uint8_t sc_audio_service_get_volume_percent(void)
{
    return 0;
}

#endif
