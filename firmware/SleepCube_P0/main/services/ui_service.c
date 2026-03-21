#include "ui_service.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "lcd_panel_if.h"
#include "lvgl_display_if.h"
#include "ui_input_buttons.h"

static const char *TAG = "sc_ui_service";

#if CONFIG_SC_UI_BACKEND_LCD_TOUCH
static void sc_ui_service_task(void *arg)
{
    (void)arg;

    esp_err_t err = sc_lcd_panel_if_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "lcd init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    err = sc_lvgl_display_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "lvgl init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    sc_lvgl_display_create_test_screen();
    vTaskDelete(NULL);
}
#endif

esp_err_t sc_ui_service_start(void)
{
#if CONFIG_SC_UI_INPUT_BUTTONS
    ESP_RETURN_ON_ERROR(sc_ui_input_buttons_start(), TAG, "button backend start failed");
#endif

#if CONFIG_SC_UI_BACKEND_LCD_TOUCH
    BaseType_t ok = xTaskCreate(sc_ui_service_task, "sc_ui", 8192, NULL, 4, NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "ui service started (LCD/touch backend)");
#else
    ESP_LOGI(TAG, "ui service enabled, LCD/touch backend not active for current board profile");
#endif
    return ESP_OK;
}
