#include "audio_fs.h"

#include <stdbool.h>
#include <dirent.h>
#include <stdio.h>
#include <strings.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "board_pins.h"

static const char *TAG = "sc_audio_fs";
static const char *SC_SPIFFS_MP3_PATH = "/spiffs/test.mp3";
static const char *SC_SD_MP3_PATH = "/sdcard/mono.mp3";

static bool s_spiffs_mounted;
static bool s_sd_mounted;
static bool s_sd_bus_initialized;
static sdmmc_card_t *s_sd_card;

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
static const int s_sd_freq_fallbacks_khz[] = { 10000, 4000, 1000 };
#define SC_SD_INIT_RETRY_COUNT     (8U)
#define SC_SD_INIT_RETRY_DELAY_MS  (250U)
#endif

static bool sc_audio_fs_is_mp3_name(const char *name)
{
    if (name == NULL) {
        return false;
    }

    const char *ext = strrchr(name, '.');
    if (ext == NULL) {
        return false;
    }
    return (strcasecmp(ext, ".mp3") == 0) || (strcasecmp(ext, ".wav") == 0);
}

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
static void sc_audio_fs_prepare_sd_pins(void)
{
    gpio_config_t cs_cfg = {
        .pin_bit_mask = (1ULL << SC_SD_CS_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    (void)gpio_config(&cs_cfg);
    (void)gpio_set_level(SC_SD_CS_GPIO, 1);

    (void)gpio_set_pull_mode(SC_LCD_DIN_GPIO, GPIO_PULLUP_ONLY);
    (void)gpio_set_pull_mode(SC_LCD_MISO_GPIO, GPIO_PULLUP_ONLY);
    (void)gpio_set_pull_mode(SC_SD_CS_GPIO, GPIO_PULLUP_ONLY);
}

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

    sc_audio_fs_prepare_sd_pins();

    esp_err_t err = ESP_OK;
    if (!s_sd_bus_initialized) {
        err = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "shared SPI bus init for SD failed: %s", esp_err_to_name(err));
            return err;
        }
        s_sd_bus_initialized = true;
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

    for (uint32_t attempt = 0; attempt < SC_SD_INIT_RETRY_COUNT; attempt++) {
        if (attempt > 0U) {
            ESP_LOGI(TAG, "SD not ready, retry %u/%u after %u ms",
                     (unsigned)attempt,
                     (unsigned)(SC_SD_INIT_RETRY_COUNT - 1U),
                     (unsigned)SC_SD_INIT_RETRY_DELAY_MS);
            vTaskDelay(pdMS_TO_TICKS(SC_SD_INIT_RETRY_DELAY_MS));
        }

        for (size_t i = 0; i < (sizeof(s_sd_freq_fallbacks_khz) / sizeof(s_sd_freq_fallbacks_khz[0])); i++) {
            host.max_freq_khz = s_sd_freq_fallbacks_khz[i];
            ESP_LOGI(TAG, "attempting SD mount at %d kHz", host.max_freq_khz);
            err = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &s_sd_card);
            if (err == ESP_OK) {
                s_sd_mounted = true;
                ESP_LOGI(TAG, "SD mounted at /sdcard (%d kHz)", host.max_freq_khz);
                sdmmc_card_print_info(stdout, s_sd_card);
                return ESP_OK;
            }

            ESP_LOGW(TAG, "SD mount attempt failed at %d kHz: %s", host.max_freq_khz, esp_err_to_name(err));
        }
    }

    ESP_LOGW(TAG, "SD mount skipped after retries: %s", esp_err_to_name(err));
    return err;
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

esp_err_t sc_audio_fs_remount_sdcard(void)
{
#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    if (s_sd_mounted) {
        return ESP_OK;
    }
    return sc_audio_fs_mount_sdcard();
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

bool sc_audio_fs_sd_mounted(void)
{
#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    return s_sd_mounted;
#else
    return false;
#endif
}

const char *sc_audio_fs_get_default_mp3_path(void)
{
    return sc_audio_fs_get_mp3_path(SC_AUDIO_MP3_SOURCE_AUTO);
}

const char *sc_audio_fs_get_mp3_path(sc_audio_mp3_source_t source)
{
#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    if ((source == SC_AUDIO_MP3_SOURCE_SD) &&
        s_sd_mounted && sc_audio_fs_path_exists(SC_SD_MP3_PATH)) {
        return SC_SD_MP3_PATH;
    }
#endif
    if ((source == SC_AUDIO_MP3_SOURCE_SPIFFS) && sc_audio_fs_path_exists(SC_SPIFFS_MP3_PATH)) {
        return SC_SPIFFS_MP3_PATH;
    }
    if (source == SC_AUDIO_MP3_SOURCE_AUTO) {
        return SC_SPIFFS_MP3_PATH;
    }
    return SC_SPIFFS_MP3_PATH;
}

bool sc_audio_fs_mp3_source_available(sc_audio_mp3_source_t source)
{
    switch (source) {
        case SC_AUDIO_MP3_SOURCE_AUTO:
            return sc_audio_fs_path_exists(SC_SPIFFS_MP3_PATH) || s_sd_mounted;
        case SC_AUDIO_MP3_SOURCE_SPIFFS:
            return sc_audio_fs_path_exists(SC_SPIFFS_MP3_PATH);
        case SC_AUDIO_MP3_SOURCE_SD:
#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
            return s_sd_mounted;
#else
            return false;
#endif
        default:
            return false;
    }
}

size_t sc_audio_fs_list_sd_tracks(char tracks[][SC_AUDIO_FS_MAX_PATH_LEN], size_t max_tracks)
{
    if ((tracks == NULL) || (max_tracks == 0U)) {
        return 0U;
    }

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    if (!s_sd_mounted) {
        return 0U;
    }

    DIR *dir = opendir("/sdcard");
    if (dir == NULL) {
        return 0U;
    }

    size_t count = 0U;
    struct dirent *entry = NULL;
    while ((count < max_tracks) && ((entry = readdir(dir)) != NULL)) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        if (!sc_audio_fs_is_mp3_name(entry->d_name)) {
            continue;
        }
        int written = snprintf(tracks[count], SC_AUDIO_FS_MAX_PATH_LEN, "/sdcard/%s", entry->d_name);
        if ((written <= 0) || ((size_t)written >= SC_AUDIO_FS_MAX_PATH_LEN)) {
            continue;
        }
        count++;
    }

    closedir(dir);
    return count;
#else
    (void)tracks;
    (void)max_tracks;
    return 0U;
#endif
}

size_t sc_audio_fs_list_sd_entries(char entries[][SC_AUDIO_FS_MAX_PATH_LEN], size_t max_entries)
{
    if ((entries == NULL) || (max_entries == 0U)) {
        return 0U;
    }

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    if (!s_sd_mounted) {
        return 0U;
    }

    DIR *dir = opendir("/sdcard");
    if (dir == NULL) {
        return 0U;
    }

    size_t count = 0U;
    struct dirent *entry = NULL;
    while ((count < max_entries) && ((entry = readdir(dir)) != NULL)) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        int written = snprintf(entries[count], SC_AUDIO_FS_MAX_PATH_LEN, "/sdcard/%s", entry->d_name);
        if ((written <= 0) || ((size_t)written >= SC_AUDIO_FS_MAX_PATH_LEN)) {
            continue;
        }
        count++;
    }

    closedir(dir);
    return count;
#else
    (void)entries;
    (void)max_entries;
    return 0U;
#endif
}

bool sc_audio_fs_resolve_sd_track(const char *selector, char *path, size_t path_len)
{
    if ((selector == NULL) || (path == NULL) || (path_len == 0U)) {
        return false;
    }

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6
    if (!s_sd_mounted) {
        return false;
    }

    if (strncmp(selector, "/sdcard/", 8) == 0) {
        if (!sc_audio_fs_path_exists(selector)) {
            return false;
        }
        int written = snprintf(path, path_len, "%s", selector);
        return (written > 0) && ((size_t)written < path_len);
    }

    if (strchr(selector, '/') == NULL) {
        char candidate[SC_AUDIO_FS_MAX_PATH_LEN];
        int written = snprintf(candidate, sizeof(candidate), "/sdcard/%s", selector);
        if ((written > 0) && ((size_t)written < sizeof(candidate)) && sc_audio_fs_path_exists(candidate)) {
            written = snprintf(path, path_len, "%s", candidate);
            return (written > 0) && ((size_t)written < path_len);
        }
    }

    return false;
#else
    (void)selector;
    (void)path;
    (void)path_len;
    return false;
#endif
}
