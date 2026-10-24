#include "light_debug_command.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static bool sc_parse_uint(const char *text, uint32_t *value)
{
    if ((text == NULL) || (value == NULL) || (*text == '\0')) {
        return false;
    }

    uint32_t parsed = 0U;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        if ((*cursor < '0') || (*cursor > '9')) {
            return false;
        }
        const uint32_t digit = (uint32_t)(*cursor - '0');
        if (parsed > ((UINT32_MAX - digit) / 10U)) {
            return false;
        }
        parsed = (parsed * 10U) + digit;
    }
    *value = parsed;
    return true;
}

static bool sc_parse_duty_bps(const char *text, uint32_t *duty_bps)
{
    if ((text == NULL) || (duty_bps == NULL) || (*text == '\0')) {
        return false;
    }

    uint32_t whole = 0U;
    uint32_t fractional = 0U;
    uint32_t fractional_digits = 0U;
    const char *cursor = text;
    while ((*cursor >= '0') && (*cursor <= '9')) {
        whole = (whole * 10U) + (uint32_t)(*cursor - '0');
        if (whole > 100U) {
            return false;
        }
        ++cursor;
    }
    if (cursor == text) {
        return false;
    }

    if (*cursor == '.') {
        ++cursor;
        while ((*cursor >= '0') && (*cursor <= '9')) {
            if (fractional_digits >= 2U) {
                return false;
            }
            fractional = (fractional * 10U) + (uint32_t)(*cursor - '0');
            ++fractional_digits;
            ++cursor;
        }
        if (fractional_digits == 0U) {
            return false;
        }
    }
    if (*cursor != '\0') {
        return false;
    }
    if (fractional_digits == 1U) {
        fractional *= 10U;
    }
    if ((whole == 100U) && (fractional != 0U)) {
        return false;
    }

    *duty_bps = (whole * 100U) + fractional;
    return true;
}

sc_light_debug_command_t sc_light_debug_parse_command(const char *line)
{
    sc_light_debug_command_t command = {
        .type = SC_LIGHT_DEBUG_COMMAND_INVALID,
        .value = 0U,
    };
    if (line == NULL) {
        return command;
    }

    char first[12] = {0};
    char second[12] = {0};
    char third[20] = {0};
    char extra[2] = {0};
    const int token_count = sscanf(line, " %11s %11s %19s %1s", first, second, third, extra);
    if (token_count == EOF || token_count == 0) {
        command.type = SC_LIGHT_DEBUG_COMMAND_EMPTY;
        return command;
    }
    if ((token_count == 1) && (strcmp(first, "help") == 0)) {
        command.type = SC_LIGHT_DEBUG_COMMAND_HELP;
        return command;
    }
    if ((token_count < 2) || (strcmp(first, "pwm") != 0)) {
        return command;
    }
    if ((token_count == 2) && (strcmp(second, "status") == 0)) {
        command.type = SC_LIGHT_DEBUG_COMMAND_STATUS;
        return command;
    }
    if ((token_count == 2) && (strcmp(second, "auto") == 0)) {
        command.type = SC_LIGHT_DEBUG_COMMAND_AUTO;
        return command;
    }
    if (token_count != 3) {
        return command;
    }

    uint32_t value = 0U;
    if ((strcmp(second, "freq") == 0) && sc_parse_uint(third, &value) &&
        (value >= 1U) && (value <= 25U)) {
        command.type = SC_LIGHT_DEBUG_COMMAND_FREQUENCY_KHZ;
        command.value = value;
    } else if ((strcmp(second, "duty") == 0) &&
               sc_parse_duty_bps(third, &value)) {
        command.type = SC_LIGHT_DEBUG_COMMAND_DUTY_BPS;
        command.value = value;
    }
    return command;
}
