#include "content_storage.h"

#include <inttypes.h>
#include <stddef.h>

#include "app_config.h"
#include "board_config.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"

static const char *TAG = "content_storage";

esp_err_t sc_content_storage_mount(sc_content_storage_t *storage)
{
    ESP_RETURN_ON_FALSE(storage != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "storage is NULL");
    ESP_RETURN_ON_FALSE(!storage->mounted && !storage->bus_initialized,
                        ESP_ERR_INVALID_STATE, TAG, "storage is already mounted");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SC_SDCARD_SPI_HOST;
    host.max_freq_khz = 20000;

    const spi_bus_config_t bus_config = {
        .mosi_io_num = SC_SDCARD_MOSI_GPIO,
        .miso_io_num = SC_SDCARD_MISO_GPIO,
        .sclk_io_num = SC_SDCARD_CLK_GPIO,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4096,
    };

    esp_err_t result = spi_bus_initialize(host.slot, &bus_config, SDSPI_DEFAULT_DMA);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(result));
        return result;
    }
    storage->bus_initialized = true;

    sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    device_config.host_id = host.slot;
    device_config.gpio_cs = SDSPI_SLOT_NO_CS;

    const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 2,
        .allocation_unit_size = 16U * 1024U,
    };

    result = esp_vfs_fat_sdspi_mount(SC_CONTENT_MOUNT_POINT,
                                     &host,
                                     &device_config,
                                     &mount_config,
                                     &storage->card);
    if (result != ESP_OK) {
        ESP_LOGE(TAG,
                 "microSD mount failed: %s; card must contain a FAT16/FAT32 filesystem",
                 esp_err_to_name(result));
        sc_content_storage_unmount(storage);
        return result;
    }
    storage->mounted = true;

    const uint64_t capacity_bytes =
        (uint64_t)storage->card->csd.capacity * storage->card->csd.sector_size;
    ESP_LOGI(TAG,
             "microSD mounted read-only-by-policy at %s: name=%s capacity=%" PRIu64
             " bytes bus=SPI%d clk=GPIO%d mosi=GPIO%d miso=GPIO%d cs=fixed-low",
             SC_CONTENT_MOUNT_POINT,
             storage->card->cid.name,
             capacity_bytes,
             (int)SC_SDCARD_SPI_HOST + 1,
             SC_SDCARD_CLK_GPIO,
             SC_SDCARD_MOSI_GPIO,
             SC_SDCARD_MISO_GPIO);
    return ESP_OK;
}

void sc_content_storage_unmount(sc_content_storage_t *storage)
{
    if (storage == NULL) {
        return;
    }

    if (storage->mounted) {
        const esp_err_t result =
            esp_vfs_fat_sdcard_unmount(SC_CONTENT_MOUNT_POINT, storage->card);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "microSD unmount failed: %s", esp_err_to_name(result));
        }
    }
    storage->mounted = false;
    storage->card = NULL;

    if (storage->bus_initialized) {
        const esp_err_t result = spi_bus_free(SC_SDCARD_SPI_HOST);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "SPI bus release failed: %s", esp_err_to_name(result));
        }
    }
    storage->bus_initialized = false;
}
