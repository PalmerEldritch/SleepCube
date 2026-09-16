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

#ifdef CONFIG_SC_LIGHT_BACKEND_LED_STRIP
#define SC_CFG_LED_STRIP 1
#else
#define SC_CFG_LED_STRIP 0
#endif

#ifdef CONFIG_SC_LED_COUNT
#define SC_CFG_LED_COUNT CONFIG_SC_LED_COUNT
#else
#define SC_CFG_LED_COUNT 0
#endif

#ifdef CONFIG_SC_LED_STRIP_GPIO
#define SC_CFG_LED_GPIO CONFIG_SC_LED_STRIP_GPIO
#else
#define SC_CFG_LED_GPIO -1
#endif

#ifdef CONFIG_SC_ENABLE_AUDIO
#define SC_CFG_AUDIO 1
#else
#define SC_CFG_AUDIO 0
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

#ifdef CONFIG_SC_LOOPBACK_ENABLE
#define SC_CFG_LOOPBACK 1
#else
#define SC_CFG_LOOPBACK 0
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
    ESP_LOGI(TAG, "services: audio=%d light=%d ui=%d",
             SC_CFG_AUDIO, SC_CFG_LIGHT, SC_CFG_UI);
    ESP_LOGI(TAG, "backends: led_strip=%d lcd_touch=%d buttons=%d loopback=%d",
             SC_CFG_LED_STRIP,
             SC_CFG_LCD_TOUCH,
             SC_CFG_UI_BUTTONS,
             SC_CFG_LOOPBACK);
    ESP_LOGI(TAG, "light cfg: led_count=%d led_gpio=%d", SC_CFG_LED_COUNT, SC_CFG_LED_GPIO);
    ESP_LOGI(TAG, "light anim: startup_ramp_ms=%d", SC_CFG_LIGHT_STARTUP_RAMP_MS);
}
