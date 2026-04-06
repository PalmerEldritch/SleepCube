#include "settings_store.h"

#include "esp_check.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "sc_settings";
static const char *SC_SETTINGS_NAMESPACE = "sleepcube";

esp_err_t sc_settings_store_init(void)
{
    esp_err_t err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "nvs erase failed");
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "nvs init failed");
    return ESP_OK;
}

esp_err_t sc_settings_store_load_u8(const char *key, uint8_t default_value, uint8_t *out)
{
    ESP_RETURN_ON_FALSE((key != NULL) && (out != NULL), ESP_ERR_INVALID_ARG, TAG, "invalid load args");

    nvs_handle_t handle;
    esp_err_t err = nvs_open(SC_SETTINGS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *out = default_value;
        return ESP_OK;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "nvs open failed");

    uint8_t value = default_value;
    err = nvs_get_u8(handle, key, &value);
    nvs_close(handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *out = default_value;
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(err, TAG, "nvs get failed");
    *out = value;
    return ESP_OK;
}

esp_err_t sc_settings_store_save_u8(const char *key, uint8_t value)
{
    ESP_RETURN_ON_FALSE(key != NULL, ESP_ERR_INVALID_ARG, TAG, "invalid save args");

    nvs_handle_t handle;
    ESP_RETURN_ON_ERROR(nvs_open(SC_SETTINGS_NAMESPACE, NVS_READWRITE, &handle), TAG, "nvs open failed");
    esp_err_t err = nvs_set_u8(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    ESP_RETURN_ON_ERROR(err, TAG, "nvs save failed");
    return ESP_OK;
}
