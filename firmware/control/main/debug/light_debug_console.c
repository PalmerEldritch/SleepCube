#include "light_debug_console.h"

#include <inttypes.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "light_debug_command.h"
#include "light_service.h"

static const char *TAG = "sc_light_debug";

static void sc_light_debug_log_help(void)
{
    ESP_LOGI(TAG, "COB PWM debug commands:");
    ESP_LOGI(TAG, "  pwm freq <1..25>    carrier frequency in integer kHz");
    ESP_LOGI(TAG, "  pwm duty <0..100>   manual duty override, up to 2 decimals");
    ESP_LOGI(TAG, "  pwm auto            release duty override; resume GUI/effects");
    ESP_LOGI(TAG, "  pwm status          show requested/applied PWM state");
    ESP_LOGI(TAG, "  help                show this command list");
}

static void sc_light_debug_log_status(void)
{
    sc_light_debug_status_t status;
    const esp_err_t err = sc_light_service_debug_get_status(&status);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "cannot read PWM status: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG,
             "PWM status: requested=%" PRIu32 "Hz actual=%" PRIu32
             "Hz resolution=%u-bit mode=%s manual_duty=%u.%02u%%",
             status.requested_frequency_hz,
             status.actual_frequency_hz,
             (unsigned)status.resolution_bits,
             status.manual_duty_enabled ? "manual" : "auto",
             (unsigned)(status.manual_duty_bps / 100U),
             (unsigned)(status.manual_duty_bps % 100U));
}

static void sc_light_debug_console_task(void *argument)
{
    (void)argument;
    char line[80];
    sc_light_debug_log_help();

    for (;;) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        const sc_light_debug_command_t command = sc_light_debug_parse_command(line);
        esp_err_t err = ESP_OK;
        switch (command.type) {
        case SC_LIGHT_DEBUG_COMMAND_EMPTY:
            break;
        case SC_LIGHT_DEBUG_COMMAND_HELP:
            sc_light_debug_log_help();
            break;
        case SC_LIGHT_DEBUG_COMMAND_STATUS:
            sc_light_debug_log_status();
            break;
        case SC_LIGHT_DEBUG_COMMAND_AUTO:
            err = sc_light_service_debug_clear_pwm_duty_override();
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "manual duty override released; GUI/effects resumed");
            }
            break;
        case SC_LIGHT_DEBUG_COMMAND_FREQUENCY_KHZ:
            err = sc_light_service_debug_set_pwm_frequency_khz((uint8_t)command.value);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "PWM frequency queued: %" PRIu32 " kHz", command.value);
            }
            break;
        case SC_LIGHT_DEBUG_COMMAND_DUTY_BPS:
            err = sc_light_service_debug_set_pwm_duty_bps((uint16_t)command.value);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "manual PWM duty queued: %" PRIu32 ".%02" PRIu32 "%%",
                         command.value / 100U, command.value % 100U);
            }
            break;
        case SC_LIGHT_DEBUG_COMMAND_INVALID:
        default:
            ESP_LOGW(TAG, "invalid PWM command; enter 'help'");
            break;
        }
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "PWM command rejected: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t sc_light_debug_console_start(void)
{
    if (xTaskCreate(sc_light_debug_console_task, "light_debug", 3072,
                    NULL, 3, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "COB PWM debug console enabled (development only)");
    return ESP_OK;
}
