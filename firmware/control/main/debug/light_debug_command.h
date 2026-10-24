#pragma once

#include <stdint.h>

typedef enum {
    SC_LIGHT_DEBUG_COMMAND_INVALID = 0,
    SC_LIGHT_DEBUG_COMMAND_EMPTY,
    SC_LIGHT_DEBUG_COMMAND_HELP,
    SC_LIGHT_DEBUG_COMMAND_STATUS,
    SC_LIGHT_DEBUG_COMMAND_AUTO,
    SC_LIGHT_DEBUG_COMMAND_FREQUENCY_KHZ,
    SC_LIGHT_DEBUG_COMMAND_DUTY_BPS,
} sc_light_debug_command_type_t;

typedef struct {
    sc_light_debug_command_type_t type;
    uint32_t value;
} sc_light_debug_command_t;

/** Parse one line of the development COB PWM console. */
sc_light_debug_command_t sc_light_debug_parse_command(const char *line);
