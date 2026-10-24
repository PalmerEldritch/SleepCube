#if CONFIG_SC_ENABLE_SEMANTIC_CONSOLE
#include <stdio.h>
#endif

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_core.h"
#include "settings_store.h"
#include "sc_audio_link.h"
#if CONFIG_SC_ENABLE_LIGHT_DEBUG_CONSOLE
#include "light_debug_console.h"
#endif

static const char *TAG = "main";

#if CONFIG_SC_ENABLE_SEMANTIC_CONSOLE
static void log_request_result(const char *name, esp_err_t result)
{
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "semantic console queued %s", name);
    } else {
        ESP_LOGW(TAG, "semantic console rejected %s: %s", name,
                 esp_err_to_name(result));
    }
}

static void semantic_console_task(void *argument)
{
    (void)argument;
    ESP_LOGI(TAG,
             "semantic diagnostic console: p=PLAY 30s (product UI=%us), "
             "s=STOP, 1/2/3/4=25/50/75/100%%, q=GET_STATUS, "
             "r=replay last command",
             (unsigned)CONFIG_SC_REMOTE_PLAY_DURATION_SECONDS);
    for (;;) {
        const int command = getchar();
        switch (command) {
        case 'p':
            log_request_result("PLAY(30s)", sc_audio_link_control_play(30U));
            break;
        case 's':
            log_request_result("STOP", sc_audio_link_control_stop());
            break;
        case '1':
        case '2':
        case '3':
        case '4': {
            const uint8_t volume = (uint8_t)(command - '0') * 25U;
            log_request_result("SET_VOLUME",
                               sc_audio_link_control_set_volume(volume));
            break;
        }
        case 'q':
            log_request_result("GET_STATUS",
                               sc_audio_link_control_request_status());
            break;
        case 'r':
            log_request_result("duplicate replay",
                               sc_audio_link_control_replay_last());
            break;
        case EOF:
            /* USB Serial/JTAG may not be open yet, or may re-enumerate after a
             * physical reset.  stdio records EOF on the FILE object, so clear
             * it before retrying or the development console remains stuck. */
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        default:
            break;
        }
    }
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "SleepCube P0 app starting");
    ESP_ERROR_CHECK(sc_settings_store_init());
    const esp_err_t link_result = sc_audio_link_start_control(16, 17);
    if (link_result != ESP_OK) {
        ESP_LOGE(TAG, "semantic audio UART unavailable: %s; UI/light continue",
                 esp_err_to_name(link_result));
    }
    ESP_ERROR_CHECK(sc_app_core_start());
#if CONFIG_SC_ENABLE_SEMANTIC_CONSOLE
    if (link_result == ESP_OK &&
        xTaskCreate(semantic_console_task, "semantic_console", 3072,
                    NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "semantic diagnostic console task unavailable");
    }
#endif
#if CONFIG_SC_ENABLE_LIGHT_DEBUG_CONSOLE
    ESP_ERROR_CHECK(sc_light_debug_console_start());
#endif
}
