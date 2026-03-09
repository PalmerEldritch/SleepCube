#include "audio_service.h"

#include <stdbool.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_log.h"

#if CONFIG_SC_ENABLE_AUDIO
#include "audio_player.h"
#include "light_service.h"
#endif

static const char *TAG = "sc_audio_service";

#define SC_AUDIO_VOLUME_STEP_PCT (5)

#if CONFIG_SC_ENABLE_AUDIO

esp_err_t sc_audio_service_start(void)
{
    ESP_LOGI(TAG, "starting audio playback service");
    ESP_RETURN_ON_ERROR(sc_audio_player_start(), TAG, "audio player start failed");
    sc_audio_player_set_enabled(false);
    sc_audio_player_set_volume_percent(70);
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

    sc_audio_player_set_enabled(enable);
    ESP_ERROR_CHECK_WITHOUT_ABORT(sc_light_service_audio_sway(enable));
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
    ESP_LOGI(TAG, "volume=%d%%", next);
    return ESP_OK;
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

#endif
