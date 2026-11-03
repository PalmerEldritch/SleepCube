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
#define SC_COB_PWM_MAX_RES_BITS  (13U)
#define SC_COB_PWM_MIN_FREQ_HZ   (1000U)
#define SC_COB_PWM_MAX_FREQ_HZ   (25000U)
#define SC_COB_PWM_CLOCK         LEDC_USE_PLL_DIV_CLK

static bool s_initialized;
static uint8_t s_resolution_bits = SC_COB_PWM_MAX_RES_BITS;
static uint32_t s_requested_frequency_hz;
static uint16_t s_last_duty_bps;

static uint32_t sc_cob_pwm_max_raw_duty(void)
{
    return (1U << s_resolution_bits) - 1U;
}

static uint32_t sc_cob_pwm_raw_duty(uint16_t duty_bps)
{
    if (duty_bps > SC_LIGHT_PWM_DUTY_BPS_MAX) {
        duty_bps = SC_LIGHT_PWM_DUTY_BPS_MAX;
    }
    return (uint32_t)(((uint64_t)duty_bps * sc_cob_pwm_max_raw_duty() +
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

    const uint8_t resolution_bits =
        sc_light_pwm_resolution_bits_for_frequency(frequency_hz);
    const ledc_timer_config_t timer_cfg = {
        .speed_mode = SC_COB_PWM_MODE,
        .duty_resolution = (ledc_timer_bit_t)resolution_bits,
        .timer_num = SC_COB_PWM_TIMER,
        .freq_hz = frequency_hz,
        /* ESP32-C6 LEDC timers share one global source. Use the 80 MHz PLL
         * for both display and COB so 25 kHz/11-bit remains achievable. */
        .clk_cfg = SC_COB_PWM_CLOCK,
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

    s_resolution_bits = resolution_bits;
    s_requested_frequency_hz = frequency_hz;
    s_last_duty_bps = 0U;
    s_initialized = true;
    ESP_LOGI(TAG,
             "COB PWM initialized: gpio=%d requested=%" PRIu32
             "Hz actual=%" PRIu32 "Hz resolution=%u-bit active_high=1",
             gpio_num, frequency_hz,
             ledc_get_freq(SC_COB_PWM_MODE, SC_COB_PWM_TIMER),
             (unsigned)s_resolution_bits);
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
        s_last_duty_bps = 0U;
    } else {
        s_last_duty_bps = duty_bps;
    }
    return err;
}

esp_err_t sc_cob_pwm_if_set_frequency_hz(uint32_t frequency_hz)
{
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "PWM not initialized");
    ESP_RETURN_ON_FALSE(frequency_hz >= SC_COB_PWM_MIN_FREQ_HZ &&
                            frequency_hz <= SC_COB_PWM_MAX_FREQ_HZ &&
                            (frequency_hz % 1000U) == 0U,
                        ESP_ERR_INVALID_ARG, TAG, "PWM frequency must be 1..25 kHz in 1 kHz steps");

    if (frequency_hz == s_requested_frequency_hz) {
        return ESP_OK;
    }

    const uint32_t old_frequency_hz = s_requested_frequency_hz;
    const uint8_t old_resolution_bits = s_resolution_bits;
    const uint16_t old_duty_bps = s_last_duty_bps;
    const uint8_t new_resolution_bits =
        sc_light_pwm_resolution_bits_for_frequency(frequency_hz);

    esp_err_t err = ledc_set_duty(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL, 0U);
    if (err == ESP_OK) {
        err = ledc_update_duty(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL);
    }
    if (err != ESP_OK) {
        (void)ledc_stop(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL, 0U);
        s_last_duty_bps = 0U;
        ESP_LOGE(TAG, "PWM safe-off before frequency change failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    const ledc_timer_config_t timer_cfg = {
        .speed_mode = SC_COB_PWM_MODE,
        .duty_resolution = (ledc_timer_bit_t)new_resolution_bits,
        .timer_num = SC_COB_PWM_TIMER,
        .freq_hz = frequency_hz,
        .clk_cfg = SC_COB_PWM_CLOCK,
    };
    err = ledc_timer_config(&timer_cfg);
    if (err != ESP_OK) {
        const ledc_timer_config_t restore_cfg = {
            .speed_mode = SC_COB_PWM_MODE,
            .duty_resolution = (ledc_timer_bit_t)old_resolution_bits,
            .timer_num = SC_COB_PWM_TIMER,
            .freq_hz = old_frequency_hz,
            .clk_cfg = SC_COB_PWM_CLOCK,
        };
        s_resolution_bits = old_resolution_bits;
        if (ledc_timer_config(&restore_cfg) == ESP_OK) {
            (void)sc_cob_pwm_if_set_duty_bps(old_duty_bps);
        } else {
            (void)ledc_stop(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL, 0U);
            s_last_duty_bps = 0U;
        }
        return err;
    }

    s_resolution_bits = new_resolution_bits;
    s_requested_frequency_hz = frequency_hz;
    err = sc_cob_pwm_if_set_duty_bps(old_duty_bps);
    if (err != ESP_OK) {
        const esp_err_t duty_err = err;
        const ledc_timer_config_t restore_cfg = {
            .speed_mode = SC_COB_PWM_MODE,
            .duty_resolution = (ledc_timer_bit_t)old_resolution_bits,
            .timer_num = SC_COB_PWM_TIMER,
            .freq_hz = old_frequency_hz,
            .clk_cfg = SC_COB_PWM_CLOCK,
        };
        s_resolution_bits = old_resolution_bits;
        s_requested_frequency_hz = old_frequency_hz;
        if ((ledc_timer_config(&restore_cfg) != ESP_OK) ||
            (sc_cob_pwm_if_set_duty_bps(old_duty_bps) != ESP_OK)) {
            (void)ledc_stop(SC_COB_PWM_MODE, SC_COB_PWM_CHANNEL, 0U);
            s_last_duty_bps = 0U;
        }
        return duty_err;
    }

    ESP_LOGI(TAG,
             "COB PWM frequency changed: requested=%" PRIu32
             "Hz actual=%" PRIu32 "Hz resolution=%u-bit duty=%u.%02u%%",
             frequency_hz,
             ledc_get_freq(SC_COB_PWM_MODE, SC_COB_PWM_TIMER),
             (unsigned)s_resolution_bits,
             (unsigned)(old_duty_bps / 100U),
             (unsigned)(old_duty_bps % 100U));
    return ESP_OK;
}

esp_err_t sc_cob_pwm_if_force_off(void)
{
    return sc_cob_pwm_if_set_duty_bps(0U);
}

uint32_t sc_cob_pwm_if_get_frequency_hz(void)
{
    if (!s_initialized) {
        return 0U;
    }
    return ledc_get_freq(SC_COB_PWM_MODE, SC_COB_PWM_TIMER);
}

uint8_t sc_cob_pwm_if_get_resolution_bits(void)
{
    return s_initialized ? s_resolution_bits : 0U;
}

uint16_t sc_cob_pwm_if_get_duty_bps(void)
{
    return s_initialized ? s_last_duty_bps : 0U;
}
