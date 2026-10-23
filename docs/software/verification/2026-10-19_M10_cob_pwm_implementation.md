# M10 R01 COB PWM Implementation Verification

**Date:** 2026-10-19

**Status:** SOFTWARE/BUILD/FLASH PASS; ELECTRICAL AND POWERED-STRIP VERIFICATION PENDING

## Scope

This record covers the software implementation and host/build evidence for the
R01 fixed-color COB lighting migration. It does not claim that the Waveshare,
MOSFET or 12 V strip has been flashed, measured or powered with this image.
Historical R00 verification reports are unchanged and do not substitute for
the R01 hardware gates.

## Implemented configuration

| Item | R01 value |
| --- | --- |
| Branch origin | `feat/r01-cob-pwm-lighting` from annotated tag `software-r00` |
| Control application version | `R01-dev` |
| MOSFET gate | Waveshare GPIO8, active high, external pull-down required |
| PWM | LEDC low-speed timer 1/channel 1, 3 kHz, 13-bit |
| Display backlight | LEDC low-speed timer 0/channel 0, unchanged |
| Logical mapping | 0% → off; 1–100% → 5–80% steady-state duty |
| Effects | global startup/smooth-change/ambient/PLAY/STOP duty effects; cap 80% |
| Removed | RGB buffers/generation, warmth/color, dithering, LED count/order, spatial animation, addressable-strip/RMT project code and dependencies |

The `light_service` API remains unchanged. The GUI, stored logical brightness,
Audio firmware and UART semantic protocol were not modified.

## Host verification

The new policy suite was built with C11, warnings as errors, pedantic checking
and UndefinedBehaviorSanitizer:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/control/main/lighting \
  firmware/control/main/lighting/light_pwm_policy.c \
  firmware/control/main/lighting/tests/test_light_pwm_policy.c \
  -lm -o /tmp/test_light_pwm_policy
/tmp/test_light_pwm_policy
```

Result: **PASS**. Cases cover valid and invalid limits, 0%, negative and NaN
fail-off, 1% = 5% duty, 50% ≈ 42.12% duty, 100% = 80% duty, upper effect
saturation, lower saturation and a 0.5% fade value = 2.5% duty below the active
floor.

Retained regressions:

| Suite | Result |
| --- | --- |
| Shared frame codec, strict C11/UBSan | PASS — `sc_frame tests passed` |
| Semantic audio protocol, strict C11/UBSan | PASS — `sc_audio_protocol tests passed` |
| Control settings policy, strict C11/UBSan | PASS — `settings policy tests passed` |
| M8 timing analyzer | PASS — 3/3 unit tests |

## Clean Control build

Implementation checkpoint `a3036fd` was clean-built with ESP-IDF v6.0 commit
`662a3be354759d9487bf4b1a629fadb766cb1800`:

```sh
cd firmware/control
source /home/jeppe/esp/v6.0/esp-idf/export.sh
idf.py -B /tmp/sleepcube-control-r01-a3036fd-build \
  -D SDKCONFIG=/tmp/sleepcube-control-r01-a3036fd-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.r01.defaults' \
  set-target esp32c6 build
```

Result: **PASS**.

| Artifact | Target/profile | Size | SHA-256 |
| --- | --- | ---: | --- |
| `SleepCube_P0.bin` | ESP32-C6/8 MB, R01 defaults | 586,640 bytes | `db5783c01517e3fffe2718f65ca9f39fb0498a5c13b57f985a302ebb2f570345` |

The image has 1,117,296 bytes (66%) free in the smallest app partition and the
build reports application version `R01-dev`. Generated configuration inspection
confirms GPIO8, 3,000 Hz, 5% floor and 80% cap.

Project-description inspection confirms the Control `main` component requires
GPIO, LCD, LVGL and LEDC-related components but no RMT component. ELF symbol
inspection with `riscv32-esp-elf-nm` finds no `rmt`, `led_strip` or
`light_engine` symbol. ESP-IDF's default non-minimal build still enumerates and
compiles its registered framework components, including unused RMT libraries;
those framework objects are not a SleepCube dependency and are not linked into
the application ELF.

Source inspection confirms `cob_pwm_if.c` uses LEDC timer 1/channel 1 at 13-bit
resolution, while `lcd_panel_if.c` retains timer 0/channel 0. The old strip
driver/encoder and RGB lighting-engine sources are deleted from this branch.

## Power and target verification state

| Evidence | State | Required next action |
| --- | --- | --- |
| Exact R01 flash and logged boot initialization with 12 V off | PASS | 2026-10-23 esptool writes hash-verified; reset boot reports `R01-dev`, GPIO8/3 kHz/13-bit, 5–80% limits and normal UART reconciliation |
| USB-powered GPIO8 reset/boot low and approximately 3 kHz duty measurements with 12 V off | PENDING | Verify external gate pull-down and explicit source-reference ground; scope off/1%/midpoint/100% |
| Powered 12 V COB behavior and effects | PENDING | Apply 12 V only after the unpowered measurement gate passes; observe flash, levels, fades, flicker, heat and noise |
| Unchanged GUI, persistence, audio/UART and Audio-disconnected behavior | PENDING | Repeat integrated product regression on R01 target |
| Final PD-trigger/DC-DC topology with USB disconnected | PENDING | Run last, after development topology passes |

During development the external DC-DC 5 V output must remain disconnected from
both controllers. The external 12 V negative/MOSFET source reference must be
connected explicitly to Waveshare ground before 12 V is applied; computer USB
grounds alone are not accepted as that reference.

## 2026-10-23 target flash record

Both controllers were connected to the laptop by USB and the user confirmed
that 12 V was disconnected. Stable USB identities resolved as:

- Waveshare Control: `AC:EB:E6:1E:67:C4` on `/dev/ttyACM0`;
- Atom Audio: `44:1B:F6:F4:FE:C8` on `/dev/ttyACM1`.

The staged application SHA-256 was rechecked as
`db5783c01517e3fffe2718f65ca9f39fb0498a5c13b57f985a302ebb2f570345`
before flashing. Esptool identified ESP32-C6FH8 revision v0.2 with embedded
8 MB flash. Bootloader, partition table and the 586,640-byte application all
passed written-data hash verification, followed by a hard reset.

A deliberate monitor reset captured a complete boot. The application reported
version `R01-dev`; the hardware profile reported COB PWM enabled on GPIO8 at
3,000 Hz with 5–80% duty limits; the driver reported 13-bit active-high PWM;
and the 50 Hz light task started with persisted brightness 57%. LCD/LVGL startup
completed. UART on Control GPIO16 TX/GPIO17 RX synchronized with the Atom and
reported authoritative `STOPPED`, persisted volume 59%, `remaining=0` and
`fault=0`.

This is firmware initialization evidence only. No instrument measurement proves
GPIO8 reset-low voltage, actual carrier frequency or physical duty yet, and no
12 V was applied.

## Disposition

M10 software, build and exact-image flash/boot initialization pass. M10-HWT-001
is partial; M10-IT-001 and M10-PWR-001 remain open. Do not create an R01 release
tag until all three target gates pass and this report is extended with measured
frequency/duty values and powered observations.
