#pragma once

#include "esp_err.h"

typedef enum {
    SC_M3_ROLE_CONTROL = 0,
    SC_M3_ROLE_AUDIO = 1,
} sc_m3_role_t;

/** Start the independent M3-only UART PING/PONG task. No audio commands exist. */
esp_err_t sc_m3_transport_start(sc_m3_role_t role, int tx_gpio, int rx_gpio);
