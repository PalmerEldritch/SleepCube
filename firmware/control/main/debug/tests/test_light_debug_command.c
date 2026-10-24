#include <assert.h>
#include <stdio.h>

#include "light_debug_command.h"

static void assert_command(const char *text,
                           sc_light_debug_command_type_t expected_type,
                           uint32_t expected_value)
{
    const sc_light_debug_command_t command = sc_light_debug_parse_command(text);
    assert(command.type == expected_type);
    assert(command.value == expected_value);
}

int main(void)
{
    assert_command("", SC_LIGHT_DEBUG_COMMAND_EMPTY, 0U);
    assert_command("  \r\n", SC_LIGHT_DEBUG_COMMAND_EMPTY, 0U);
    assert_command("help\n", SC_LIGHT_DEBUG_COMMAND_HELP, 0U);
    assert_command("pwm status\n", SC_LIGHT_DEBUG_COMMAND_STATUS, 0U);
    assert_command("pwm auto\n", SC_LIGHT_DEBUG_COMMAND_AUTO, 0U);

    assert_command("pwm freq 1\n", SC_LIGHT_DEBUG_COMMAND_FREQUENCY_KHZ, 1U);
    assert_command("pwm freq 25\n", SC_LIGHT_DEBUG_COMMAND_FREQUENCY_KHZ, 25U);
    assert_command("pwm freq 0\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);
    assert_command("pwm freq 26\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);
    assert_command("pwm freq 3.0\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);

    assert_command("pwm duty 0\n", SC_LIGHT_DEBUG_COMMAND_DUTY_BPS, 0U);
    assert_command("pwm duty 5\n", SC_LIGHT_DEBUG_COMMAND_DUTY_BPS, 500U);
    assert_command("pwm duty 42.1\n", SC_LIGHT_DEBUG_COMMAND_DUTY_BPS, 4210U);
    assert_command("pwm duty 42.12\n", SC_LIGHT_DEBUG_COMMAND_DUTY_BPS, 4212U);
    assert_command("pwm duty 100.00\n", SC_LIGHT_DEBUG_COMMAND_DUTY_BPS, 10000U);
    assert_command("pwm duty 100.01\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);
    assert_command("pwm duty -1\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);
    assert_command("pwm duty 1.234\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);
    assert_command("pwm duty 10 extra\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);
    assert_command("pwm banana 10\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);
    assert_command("garbage\n", SC_LIGHT_DEBUG_COMMAND_INVALID, 0U);

    puts("light debug command tests passed");
    return 0;
}
