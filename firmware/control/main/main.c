#include "esp_check.h"
#include "esp_log.h"
#include "app_core.h"
#include "settings_store.h"
#include "sc_m3_transport.h"

void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "SleepCube P0 app starting");
    ESP_ERROR_CHECK(sc_settings_store_init());
    ESP_ERROR_CHECK(sc_app_core_start());
    const esp_err_t link_result = sc_m3_transport_start(SC_M3_ROLE_CONTROL, 16, 17);
    if (link_result != ESP_OK) {
        ESP_LOGE(TAG, "M3 diagnostic UART unavailable: %s; local UI/light continue",
                 esp_err_to_name(link_result));
    }
}
