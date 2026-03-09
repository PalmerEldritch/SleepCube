#include "app_core.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "audio_service.h"
#include "light_service.h"
#include "ui_service.h"
#include "hw_profile.h"
#include "sc_trace.h"

static const char *TAG = "sc_app_core";

static QueueHandle_t s_event_queue;

esp_err_t sc_app_core_post_event(const sc_app_event_t *event)
{
    if ((s_event_queue == NULL) || (event == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(s_event_queue, event, 0) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static void sc_app_core_task(void *arg)
{
    (void)arg;
    sc_app_event_t event;

    while (1) {
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        SC_TRACE_MARK("app_core", "evt_rx", (int32_t)event.type);
        SC_TRACE_MARK("app_core", "dispatch_start", (int32_t)event.type);

        switch (event.type) {
            case SC_APP_EVT_BOOT:
                ESP_LOGI(TAG, "boot event handled");
                break;
            case SC_APP_EVT_UI_AUDIO_TOGGLE:
                // Touch UI semantics: single-tap toggles music playback.
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_service_toggle_playback());
                break;
            case SC_APP_EVT_UI_AUDIO_STOP:
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_service_set_playback(false));
                break;
            case SC_APP_EVT_UI_VOLUME_STEP:
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_service_change_volume((int)event.value));
                break;
            case SC_APP_EVT_UI_LIGHT_STEP:
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_light_service_change_brightness((int)event.value));
                break;
            case SC_APP_EVT_UI_LIGHT_TOGGLE:
                ESP_ERROR_CHECK_WITHOUT_ABORT(sc_light_service_toggle());
                break;
            default:
                break;
        }
        SC_TRACE_MARK("app_core", "dispatch_end", (int32_t)event.type);
    }
}

esp_err_t sc_app_core_start(void)
{
    sc_hw_profile_log();

    s_event_queue = xQueueCreate(8, sizeof(sc_app_event_t));
    if (s_event_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(sc_app_core_task, "sc_app_core", 4096, NULL, 6, NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }

#if CONFIG_SC_ENABLE_AUDIO
    ESP_RETURN_ON_ERROR(sc_audio_service_start(), TAG, "audio service start failed");
#else
    ESP_LOGI(TAG, "audio service disabled");
#endif

#if CONFIG_SC_ENABLE_LIGHT
    ESP_RETURN_ON_ERROR(sc_light_service_start(), TAG, "light service start failed");
#else
    ESP_LOGI(TAG, "light service disabled");
#endif

#if CONFIG_SC_ENABLE_UI
    ESP_RETURN_ON_ERROR(sc_ui_service_start(), TAG, "ui service start failed");
#else
    ESP_LOGI(TAG, "ui service disabled");
#endif

    sc_app_event_t boot_evt = { .type = SC_APP_EVT_BOOT, .value = 0 };
    (void)sc_app_core_post_event(&boot_evt);
    return ESP_OK;
}
