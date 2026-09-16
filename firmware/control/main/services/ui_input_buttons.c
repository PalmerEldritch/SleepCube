#include "ui_input_buttons.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "app_core.h"
#include "sc_trace.h"

static const char *TAG = "sc_ui_buttons";

#define SC_BTN_POLL_MS             (20)

typedef struct {
    gpio_num_t gpio;
    bool prev_pressed;
} sc_btn_t;

static bool sc_btn_read_pressed(gpio_num_t gpio)
{
    return gpio_get_level(gpio) == 0;
}

static esp_err_t sc_btn_emit(sc_app_event_type_t type, int32_t value)
{
    sc_app_event_t evt = {
        .type = type,
        .value = value,
    };
    SC_TRACE_MARK("ui_btn", "emit", (int32_t)type);
    return sc_app_core_post_event(&evt);
}

static void sc_ui_input_buttons_task(void *arg)
{
    (void)arg;
    sc_btn_t btn_audio = { .gpio = (gpio_num_t)CONFIG_SC_BTN_AUDIO_GPIO, .prev_pressed = false };
    sc_btn_t btn_vol_up = { .gpio = (gpio_num_t)CONFIG_SC_BTN_VOL_UP_GPIO, .prev_pressed = false };
    sc_btn_t btn_vol_down = { .gpio = (gpio_num_t)CONFIG_SC_BTN_VOL_DOWN_GPIO, .prev_pressed = false };
    sc_btn_t btn_light_up = { .gpio = (gpio_num_t)CONFIG_SC_BTN_LIGHT_UP_GPIO, .prev_pressed = false };
    sc_btn_t btn_light_down = { .gpio = (gpio_num_t)CONFIG_SC_BTN_LIGHT_DOWN_GPIO, .prev_pressed = false };

    while (1) {
        const bool audio_pressed = sc_btn_read_pressed(btn_audio.gpio);
        if (audio_pressed && !btn_audio.prev_pressed) {
            (void)sc_btn_emit(SC_APP_EVT_UI_AUDIO_TOGGLE, 0);
            ESP_LOGI(TAG, "audio button: single press -> toggle");
        }
        btn_audio.prev_pressed = audio_pressed;

        const bool vol_up_pressed = sc_btn_read_pressed(btn_vol_up.gpio);
        if (vol_up_pressed && !btn_vol_up.prev_pressed) {
            (void)sc_btn_emit(SC_APP_EVT_UI_VOLUME_STEP, +1);
            ESP_LOGI(TAG, "volume up");
        }
        btn_vol_up.prev_pressed = vol_up_pressed;

        const bool vol_down_pressed = sc_btn_read_pressed(btn_vol_down.gpio);
        if (vol_down_pressed && !btn_vol_down.prev_pressed) {
            (void)sc_btn_emit(SC_APP_EVT_UI_VOLUME_STEP, -1);
            ESP_LOGI(TAG, "volume down");
        }
        btn_vol_down.prev_pressed = vol_down_pressed;

        const bool light_up_pressed = sc_btn_read_pressed(btn_light_up.gpio);
        if (light_up_pressed && !btn_light_up.prev_pressed) {
            (void)sc_btn_emit(SC_APP_EVT_UI_LIGHT_STEP, +1);
            ESP_LOGI(TAG, "light up");
        }
        btn_light_up.prev_pressed = light_up_pressed;

        const bool light_down_pressed = sc_btn_read_pressed(btn_light_down.gpio);
        if (light_down_pressed && !btn_light_down.prev_pressed) {
            (void)sc_btn_emit(SC_APP_EVT_UI_LIGHT_STEP, -1);
            ESP_LOGI(TAG, "light down");
        }
        btn_light_down.prev_pressed = light_down_pressed;

        vTaskDelay(pdMS_TO_TICKS(SC_BTN_POLL_MS));
    }
}

esp_err_t sc_ui_input_buttons_start(void)
{
    const gpio_num_t gpios[] = {
        (gpio_num_t)CONFIG_SC_BTN_AUDIO_GPIO,
        (gpio_num_t)CONFIG_SC_BTN_VOL_UP_GPIO,
        (gpio_num_t)CONFIG_SC_BTN_VOL_DOWN_GPIO,
        (gpio_num_t)CONFIG_SC_BTN_LIGHT_UP_GPIO,
        (gpio_num_t)CONFIG_SC_BTN_LIGHT_DOWN_GPIO,
    };

    for (size_t i = 0; i < (sizeof(gpios) / sizeof(gpios[0])); i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = (1ULL << gpios[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        if (gpio_config(&cfg) != ESP_OK) {
            return ESP_FAIL;
        }
    }

    BaseType_t ok = xTaskCreate(sc_ui_input_buttons_task, "sc_ui_btn", 4096, NULL, 5, NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "button input backend started");
    ESP_LOGI(TAG, "GPIO map audio=%d vol_up=%d vol_down=%d light_up=%d light_down=%d",
             CONFIG_SC_BTN_AUDIO_GPIO,
             CONFIG_SC_BTN_VOL_UP_GPIO,
             CONFIG_SC_BTN_VOL_DOWN_GPIO,
             CONFIG_SC_BTN_LIGHT_UP_GPIO,
             CONFIG_SC_BTN_LIGHT_DOWN_GPIO);
    return ESP_OK;
}
