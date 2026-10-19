#include "hw_profile.h"

#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "sc_hw_profile";

#ifdef CONFIG_SC_UI_BACKEND_LCD_TOUCH
#define SC_CFG_LCD_TOUCH 1
#else
#define SC_CFG_LCD_TOUCH 0
#endif

#ifdef CONFIG_SC_UI_INPUT_BUTTONS
#define SC_CFG_UI_BUTTONS 1
#else
#define SC_CFG_UI_BUTTONS 0
#endif

#ifdef CONFIG_SC_LIGHT_PWM_GPIO
#define SC_CFG_LIGHT_PWM_GPIO CONFIG_SC_LIGHT_PWM_GPIO
#else
#define SC_CFG_LIGHT_PWM_GPIO -1
#endif

#ifdef CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ
#define SC_CFG_LIGHT_PWM_FREQUENCY_HZ CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ
#else
#define SC_CFG_LIGHT_PWM_FREQUENCY_HZ 0
#endif

#ifdef CONFIG_SC_LIGHT_PWM_MIN_DUTY_PCT
#define SC_CFG_LIGHT_PWM_MIN_DUTY_PCT CONFIG_SC_LIGHT_PWM_MIN_DUTY_PCT
#else
#define SC_CFG_LIGHT_PWM_MIN_DUTY_PCT 0
#endif

#ifdef CONFIG_SC_LIGHT_PWM_MAX_DUTY_PCT
#define SC_CFG_LIGHT_PWM_MAX_DUTY_PCT CONFIG_SC_LIGHT_PWM_MAX_DUTY_PCT
#else
#define SC_CFG_LIGHT_PWM_MAX_DUTY_PCT 0
#endif

#ifdef CONFIG_SC_ENABLE_LIGHT
#define SC_CFG_LIGHT 1
#else
#define SC_CFG_LIGHT 0
#endif

#ifdef CONFIG_SC_ENABLE_UI
#define SC_CFG_UI 1
#else
#define SC_CFG_UI 0
#endif

#ifdef CONFIG_SC_LIGHT_STARTUP_RAMP_MS
#define SC_CFG_LIGHT_STARTUP_RAMP_MS CONFIG_SC_LIGHT_STARTUP_RAMP_MS
#else
#define SC_CFG_LIGHT_STARTUP_RAMP_MS 0
#endif

void sc_hw_profile_log(void)
{
#if CONFIG_SC_BOARD_DEVKITC_ESP32
    const char *board = "ESP32 DevKitC";
#elif CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    const char *board = "Waveshare ESP32-C6 Touch Display";
#else
    const char *board = "Unknown";
#endif

    ESP_LOGI(TAG, "board profile: %s", board);
    ESP_LOGI(TAG, "services: audio=remote light=%d ui=%d",
             SC_CFG_LIGHT, SC_CFG_UI);
    ESP_LOGI(TAG, "backends: cob_pwm=%d lcd_touch=%d buttons=%d",
             SC_CFG_LIGHT,
             SC_CFG_LCD_TOUCH,
             SC_CFG_UI_BUTTONS);
    ESP_LOGI(TAG, "light cfg: pwm_gpio=%d frequency=%dHz duty=%d..%d%%",
             SC_CFG_LIGHT_PWM_GPIO,
             SC_CFG_LIGHT_PWM_FREQUENCY_HZ,
             SC_CFG_LIGHT_PWM_MIN_DUTY_PCT,
             SC_CFG_LIGHT_PWM_MAX_DUTY_PCT);
    ESP_LOGI(TAG, "light anim: startup_ramp_ms=%d", SC_CFG_LIGHT_STARTUP_RAMP_MS);
}
