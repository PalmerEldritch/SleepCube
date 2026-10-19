#include "cob_pwm_if.h"

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"
#include "light_pwm_policy.h"

static const char *TAG = "sc_cob_pwm_if";

#define SC_COB_PWM_MODE          LEDC_LOW_SPEED_MODE
#define SC_COB_PWM_TIMER         LEDC_TIMER_1
#define SC_COB_PWM_CHANNEL       LEDC_CHANNEL_1
#define SC_COB_PWM_DUTY_RES      LEDC_TIMER_13_BIT
#define SC_COB_PWM_MAX_RAW_DUTY  ((1U << 13U) - 1U)

static bool s_initialized;

static uint32_t sc_cob_pwm_raw_duty(uint16_t duty_bps)
{
    if (duty_bps > SC_LIGHT_PWM_DUTY_BPS_MAX) {
        duty_bps = SC_LIGHT_PWM_DUTY_BPS_MAX;
    }
    return (uint32_t)(((uint64_t)duty_bps * SC_COB_PWM_MAX_RAW_DUTY +
                       (SC_LIGHT_PWM_DUTY_BPS_MAX / 2U)) /
                      SC_LIGHT_PWM_DUTY_BPS_MAX);
}

esp_err_t sc_cob_pwm_if_init(int gpio_num, uint32_t frequency_hz)
{
    ESP_RETURN_ON_FALSE(GPIO_IS_VALID_OUTPUT_GPIO(gpio_num), ESP_ERR_INVALID_ARG,
                        TAG, "invalid PWM GPIO");
    ESP_RETURN_ON_FALSE(frequency_hz > 0U, ESP_ERR_INVALID_ARG,
                        TAG, "invalid PWM frequency");

    const gpio_config_t safe_gpio_cfg = {
        .pin_bit_mask = 1ULL << (uint32_t)gpio_num,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&safe_gpio_cfg), TAG, "safe GPIO config failed");
    ESP_RETURN_ON_ERROR(gpio_set_level((gpio_num_t)gpio_num, 0), TAG, "safe GPIO low failed");

    const ledc_timer_config_t timer_cfg = {
        .speed_mode = SC_COB_PWM_MODE,
        .duty_resolution = SC_COB_PWM_DUTY_RES,
        .timer_num = SC_COB_PWM_TIMER,
        .freq_hz = frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) {
        (void)gpio_set_level((gpio_num_t)gpio_num, 0);
        return err;
    }

    const ledc_channel_config_t channel_cfg = {
        .gpio_num = gpio_num,
        .speed_mode = SC_COB_PWM_MODE,
        .channel = SC_COB_PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = SC_COB_PWM_TIMER,
        .duty = 0U,
        .hpoint = 0U,
    };
    err = ledc_channel_config(&channel_cfg);
    if (err != ESP_OK) {
        (void)gpio_set_level((gpio_num_t)gpio_num, 0);
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "COB PWM initialized: gpio=%d frequency=%" PRIu32 "Hz resolution=13-bit active_high=1",
             gpio_num, frequency_hz);
    return ESP_OK;
}

esp_err_t sc_cob_pwm_if_set_duty_bps(uint16_t duty_bps)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "PWM not initialized");
    ESP_RETURN_ON_FALSE(duty_bps <= SC_LIGHT_PWM_DUTY_BPS_MAX, ESP_ERR_INVALID_ARG,
                        TAG, "PWM duty out of range");

    const uint32_t raw_duty = sc_cob_pwm_raw_duty(duty_bps);
    esp_err_t err = ledc_set_duty(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL, raw_duty);
    if (err == ESP_OK) {
        err = ledc_update_duty(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL);
    }
    if (err != ESP_OK) {
        (void)ledc_stop(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL, 0U);
    }
    return err;
}

esp_err_t sc_cob_pwm_if_force_off(void)
{
    return sc_cob_pwm_if_set_duty_bps(0U);
}
