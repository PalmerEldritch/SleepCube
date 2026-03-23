#include "audio_fs.h"

#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include "esp_check.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_master.h"
#include "board_pins.h"

static const char *TAG = "sc_audio_fs";
static const char *SC_SPIFFS_MP3_PATH = "/spiffs/test.mp3";
static const char *SC_SD_MP3_PATH = "/sdcard/test.mp3";

static bool s_spiffs_mounted;
static bool s_sd_mounted;
static sdmmc_card_t *s_sd_card;

static bool sc_audio_fs_path_exists(const char *path)
{
    struct stat st = {0};
    return stat(path, &st) == 0;
}

static esp_err_t sc_audio_fs_mount_spiffs(void)
{
    if (s_spiffs_mounted) {
        return ESP_OK;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 4,
        .format_if_mount_failed = false,
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        return err;
    }

    size_t total = 0;
    size_t used = 0;
    ESP_RETURN_ON_ERROR(esp_spiffs_info(conf.partition_label, &total, &used), TAG, "spiffs info failed");
    ESP_LOGI(TAG, "SPIFFS mounted: total=%u used=%u", (unsigned)total, (unsigned)used);
    s_spiffs_mounted = true;
    return ESP_OK;
}

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
static esp_err_t sc_audio_fs_mount_sdcard(void)
{
    if (s_sd_mounted) {
        return ESP_OK;
    }

    const spi_bus_config_t bus_cfg = {
        .sclk_io_num = SC_LCD_CLK_GPIO,
        .mosi_io_num = SC_LCD_DIN_GPIO,
        .miso_io_num = SC_LCD_MISO_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SC_LCD_SPI_HOST;

    esp_err_t err = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "shared SPI bus init for SD failed: %s", esp_err_to_name(err));
        return err;
    }

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SC_SD_CS_GPIO;
    slot_cfg.host_id = host.slot;

    const esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false,
        .use_one_fat = false,
    };

    err = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &s_sd_card);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD mount skipped: %s", esp_err_to_name(err));
        return err;
    }

    s_sd_mounted = true;
    ESP_LOGI(TAG, "SD mounted at /sdcard");
    sdmmc_card_print_info(stdout, s_sd_card);
    return ESP_OK;
}
#endif

esp_err_t sc_audio_fs_mount(void)
{
    ESP_RETURN_ON_ERROR(sc_audio_fs_mount_spiffs(), TAG, "SPIFFS mount failed");

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    (void)sc_audio_fs_mount_sdcard();
#endif

    return ESP_OK;
}

const char *sc_audio_fs_get_default_mp3_path(void)
{
#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    if (s_sd_mounted && sc_audio_fs_path_exists(SC_SD_MP3_PATH)) {
        return SC_SD_MP3_PATH;
    }
#endif
    return SC_SPIFFS_MP3_PATH;
}
