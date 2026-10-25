# SleepCube R01 Control Controller

ESP-IDF firmware for the Waveshare ESP32-C6 touch/display controller. This R01
development variant preserves the R00 GUI, brightness persistence and semantic
UART Audio facade while replacing the addressable RGB strip with a fixed-color
12 V COB strip controlled through an active-high logic-level MOSFET.

The R00 firmware remains available unchanged at tag `software-r00`. This branch
is COB-only; it contains no Control-side RGB/RMT lighting implementation.

## Toolchain and signal mapping

- ESP-IDF v6.0, commit `662a3be354759d9487bf4b1a629fadb766cb1800`
- target: `esp32c6`, configured flash size: 8 MB
- COB MOSFET gate: Waveshare GPIO8, active high
- COB PWM: 3 kHz, 13-bit, LEDC timer 1/channel 1
- display backlight: LEDC timer 0/channel 0
- both LEDC timers share the fixed 80 MHz PLL-divided source required by ESP32-C6
- Control GPIO16 TX → Audio GPIO2 RX
- Audio GPIO1 TX → Control GPIO17 RX
- UART/common ground retained; Atom Grove 5 V left disconnected

The MOSFET must be suitable for a 3.3 V gate drive and must have an external
GPIO8 gate pull-down. GPIO8 is an ESP32-C6 strapping pin, so normal reset/boot
with the completed gate circuit must be verified before the strip is energized.

## Power arrangements

During development:

- power Waveshare and Atom only from their respective computer USB connections;
- use the external supply only for the COB 12 V rail;
- connect COB positive to +12 V and COB negative through the MOSFET drain/source
  switching path;
- connect the external 12 V negative/MOSFET source reference explicitly to
  Waveshare ground before applying 12 V;
- retain the UART/common-ground wiring between controllers;
- do not connect the external DC-DC 5 V output to either controller.

Do not rely only on computer USB grounds for the MOSFET gate-source reference.

For the final product, the USB-C PD trigger supplies the common 12 V rail and
the DC-DC converter supplies both controller 5 V pins. The Atom Grove 5 V
conductor remains unused. Disconnect computer USB power before energizing the
external 5 V arrangement unless both boards' simultaneous-power paths have
separately been verified.

## Clean R01 build

Source ESP-IDF, then use the dedicated R01 profile:

```sh
source /home/jeppe/esp/v6.0/esp-idf/export.sh
idf.py -B /tmp/sleepcube-control-r01-build \
  -D SDKCONFIG=/tmp/sleepcube-control-r01-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.r01.defaults' \
  set-target esp32c6 build
```

The profile selects GPIO8, 3 kHz, a 5% active-duty floor, an 80% cap, the
normal finite 3,600-second Audio request, and application version `R01-dev`.
The retained normal GUI has a nonzero minimum and does not expose lighting off.
Internal brightness 0% is fully off; nonzero values map to 5–80% steady-state
duty. Startup and internal transition-to-off fades may pass below 5%, and all
effects are capped at 80%. Invalid floor/cap configuration forces the output
off and rejects lighting-service startup.

For timing capture, append `sdkconfig.m7.trace` to the R01 defaults list. The
trace overlay changes diagnostics only; it does not restore the old RGB backend.

## R01 COB PWM development console

The `R01-dev` profile enables a USB Serial/JTAG console for controlled lighting
experiments. Enter complete lines in `idf.py monitor`:

```text
pwm freq 1       # select 1 kHz
pwm freq 25      # select 25 kHz
pwm duty 42.12   # hold an exact requested 42.12% duty
pwm duty 0       # hold output low
pwm duty 100     # request maximum duty
pwm status       # show requested/actual frequency, resolution and mode
pwm auto         # release duty override and resume GUI/effects
help
```

Frequency accepts integer 1 kHz steps from 1 through 25 kHz and remains selected
until another command or reset. Reset restores the configured 3 kHz default.
The driver retains 13-bit resolution through 9 kHz, uses 12 bits from 10 through
19 kHz and 11 bits from 20 through 25 kHz. Duty accepts 0.01% input units but is
quantized to the active hardware resolution. Manual duty mode intentionally
bypasses the normal 5–80% product limits and all ambient/PLAY/STOP effects; GUI
brightness changes continue to update the normal state and take effect after
`pwm auto`. The override is volatile and clears on reset.

`CONFIG_SC_ENABLE_LIGHT_DEBUG_CONSOLE` is development-only and shall be disabled
before an R01 release image is created. It is mutually exclusive with the older
M4 semantic-audio console so only one task reads USB stdin.

## Host tests

Run the COB mapping policy test:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/control/main/lighting \
  firmware/control/main/lighting/light_pwm_policy.c \
  firmware/control/main/lighting/tests/test_light_pwm_policy.c \
  -lm -o /tmp/test_light_pwm_policy
/tmp/test_light_pwm_policy
```

Run the development-console parser test:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/control/main/debug \
  firmware/control/main/debug/light_debug_command.c \
  firmware/control/main/debug/tests/test_light_debug_command.c \
  -o /tmp/test_light_debug_command
/tmp/test_light_debug_command
```

Run timing-analyzer regressions from the repository root with:

```sh
python3 -m unittest discover -s firmware/control/tools -p 'test_*.py' -v
```

## Flash and staged hardware verification

Identify the Waveshare by its stable entry under `/dev/serial/by-id/`; do not
assume `/dev/ttyACM0`.

```sh
idf.py -B /tmp/sleepcube-control-r01-build \
  -D SDKCONFIG=/tmp/sleepcube-control-r01-sdkconfig \
  -p /dev/ttyACM0 flash

idf.py -B /tmp/sleepcube-control-r01-build \
  -D SDKCONFIG=/tmp/sleepcube-control-r01-sdkconfig \
  -p /dev/ttyACM0 monitor --no-reset
```

With the external 12 V supply off, first verify GPIO8 stays low through reset
and produces approximately 3 kHz at expected duties. Then apply 12 V and check
for no boot flash, correct off/minimum/midpoint/maximum output, smooth fades and
effects, flicker, MOSFET heating and PWM-related noise. Repeat the GUI,
persistence, UART/audio and Audio-disconnected regressions. The final
PD-trigger/DC-DC power-cycle smoke is a separate last step with computer USB
power disconnected.

Do not create an R01 release tag until the target and final-power gates pass.
