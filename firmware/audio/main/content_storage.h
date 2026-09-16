#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "sdmmc_cmd.h"

typedef struct {
    sdmmc_card_t *card;
    bool bus_initialized;
    bool mounted;
} sc_content_storage_t;

/** Mount the Speaker Base microSD card read-only by application policy. */
esp_err_t sc_content_storage_mount(sc_content_storage_t *storage);

/** Unmount the card and release its SPI bus. Safe after partial init. */
void sc_content_storage_unmount(sc_content_storage_t *storage);
