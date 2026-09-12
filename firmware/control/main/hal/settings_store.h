#pragma once

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize non-volatile settings storage.
 *
 * This helper owns the SleepCube NVS namespace setup used by services for
 * persistent user settings such as brightness and volume.
 */
esp_err_t sc_settings_store_init(void);

/**
 * @brief Load an unsigned 8-bit setting from SleepCube NVS.
 *
 * If the key does not exist yet, @p default_value is returned through @p out.
 */
esp_err_t sc_settings_store_load_u8(const char *key, uint8_t default_value, uint8_t *out);

/**
 * @brief Persist an unsigned 8-bit setting to SleepCube NVS.
 */
esp_err_t sc_settings_store_save_u8(const char *key, uint8_t value);
