#include "esp_check.h"
#include "esp_log.h"
#include "app_core.h"
#include "settings_store.h"

void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "SleepCube P0 app starting");
    ESP_ERROR_CHECK(sc_settings_store_init());
    ESP_ERROR_CHECK(sc_app_core_start());
}
