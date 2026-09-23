#include "audio_settings.h"

#include "app_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "audio_settings";
static const char *NVS_NAMESPACE = "sleepcube";
static const char *NVS_VOLUME_KEY = "volume";

esp_err_t sc_audio_settings_init(uint8_t *volume_percent)
{
    ESP_RETURN_ON_FALSE(volume_percent != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "volume output is NULL");

    const esp_err_t init_result = nvs_flash_init();
    if (init_result != ESP_OK) {
        ESP_LOGE(TAG,
                 "NVS initialization failed: %s; refusing to erase persisted data automatically",
                 esp_err_to_name(init_result));
        return init_result;
    }

    nvs_handle_t handle;
    esp_err_t result = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (result == ESP_ERR_NVS_NOT_FOUND) {
        *volume_percent = SC_VOLUME_DEFAULT_PERCENT;
        ESP_LOGI(TAG, "no saved volume; using default=%u%%", *volume_percent);
        return ESP_OK;
    }
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "cannot open settings: %s", esp_err_to_name(result));
        return result;
    }

    uint8_t saved_volume = SC_VOLUME_DEFAULT_PERCENT;
    result = nvs_get_u8(handle, NVS_VOLUME_KEY, &saved_volume);
    nvs_close(handle);
    if (result == ESP_ERR_NVS_NOT_FOUND) {
        *volume_percent = SC_VOLUME_DEFAULT_PERCENT;
        ESP_LOGI(TAG, "no saved volume; using default=%u%%", *volume_percent);
        return ESP_OK;
    }
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "saved volume read failed: %s", esp_err_to_name(result));
        return result;
    }

    if (saved_volume > SC_VOLUME_MAX_PERCENT) {
        ESP_LOGW(TAG, "saved volume=%u%% is invalid; clamped to %u%%",
                 saved_volume, SC_VOLUME_MAX_PERCENT);
        saved_volume = SC_VOLUME_MAX_PERCENT;
        const esp_err_t repair_result =
            sc_audio_settings_save_volume(saved_volume);
        if (repair_result != ESP_OK) {
            ESP_LOGW(TAG, "clamped volume could not be repaired in NVS: %s",
                     esp_err_to_name(repair_result));
        }
    }
    *volume_percent = saved_volume;
    ESP_LOGI(TAG, "loaded saved volume=%u%%", *volume_percent);
    return ESP_OK;
}

esp_err_t sc_audio_settings_save_volume(uint8_t volume_percent)
{
    ESP_RETURN_ON_FALSE(volume_percent <= SC_VOLUME_MAX_PERCENT,
                        ESP_ERR_INVALID_ARG, TAG, "volume is out of range");

    nvs_handle_t handle;
    esp_err_t result = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "cannot open settings for write: %s", esp_err_to_name(result));
        return result;
    }

    uint8_t saved_volume = 0;
    result = nvs_get_u8(handle, NVS_VOLUME_KEY, &saved_volume);
    if (result == ESP_OK && saved_volume == volume_percent) {
        nvs_close(handle);
        return ESP_OK;
    }
    if (result != ESP_OK && result != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        ESP_LOGE(TAG, "saved volume comparison failed: %s", esp_err_to_name(result));
        return result;
    }

    result = nvs_set_u8(handle, NVS_VOLUME_KEY, volume_percent);
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "volume persistence failed: %s", esp_err_to_name(result));
    }
    return result;
}
