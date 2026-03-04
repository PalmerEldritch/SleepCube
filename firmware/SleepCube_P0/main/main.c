#include "esp_check.h"
#include "esp_log.h"
#include "audio_player.h"

void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "SleepCube P0 MP3 over I2S test starting");
    ESP_ERROR_CHECK(sc_audio_player_start());
}
