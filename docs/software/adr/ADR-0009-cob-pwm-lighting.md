# ADR-0009 — Fixed-color COB PWM lighting for R01

**Status:** Accepted

**Date:** 2026-10-19

**Supersedes:** RGB-specific portions of ADR-0002 and ADR-0003

**Superseded by:** ADR-0010 (normal-GUI off-selection clause only)

## Context

R00 drives a 5 V addressable RGB strip from the Control Controller with RMT,
per-pixel buffers, color/warmth modulation and spatial animation. The R01
hardware replaces that strip with a fixed-color 12 V COB strip switched on its
low side by a 3.3 V-compatible logic-level MOSFET. The established GUI,
brightness persistence, controller ownership and semantic Audio UART contract
do not need to change.

The Waveshare display backlight already uses LEDC timer 0/channel 0. GPIO8 is
available for the lighting output but is an ESP32-C6 strapping pin, so reset and
boot behavior require explicit fail-safe handling and target verification. The
ESP32-C6 timers have separate timer/channel resources but share one LEDC global
clock source.

## Decision

The R01 Control firmware is a COB-only variant on branch
`feat/r01-cob-pwm-lighting`, derived from immutable release tag `software-r00`.
It shall:

- drive the external MOSFET gate active-high on Waveshare GPIO8 with LEDC
  timer 1/channel 1 at 3 kHz and 13-bit resolution;
- retain display timer 0/channel 0 and select the fixed 80 MHz PLL-divided LEDC
  source for both timers so runtime COB frequency tests cannot retune the shared
  clock domain;
- initialize at zero duty and force zero duty on output errors;
- retain the public `light_service` API, UI layout, stored 0–100% brightness
  value, startup fade, smooth changes, low-frequency ambient fluctuation and
  PLAY/STOP attack/release responses;
- map 0% to fully off with effects suppressed, and map 1–100% linearly to a
  configurable 5–80% steady-state duty range;
- permit startup and transition-to-off fades below the 5% active floor, while
  capping all output and effects at 80%;
- reject an invalid duty floor/cap combination and keep the output off; and
- remove Control-side RGB generation, color/warmth processing, dithering,
  per-pixel animation, LED ordering/count handling and project RMT dependency.

The GUI, Audio Controller firmware and semantic UART protocol remain unchanged.

During development, the Waveshare and Atom shall each be powered only from its
computer USB connection. The external supply shall feed only the COB 12 V rail;
its negative/MOSFET source reference shall connect explicitly to Waveshare
ground, and the external DC-DC 5 V output shall remain disconnected.

In the final product, the PD trigger shall create the common 12 V rail and the
DC-DC converter shall supply both controllers through their 5 V pins. The Atom
Grove 5 V conductor remains unused. Computer USB power shall be disconnected
before the external 5 V arrangement is energized unless simultaneous power
paths have separately been verified.

An external GPIO8 gate pull-down is mandatory in both arrangements. Software
pull-down configuration is supplemental and does not protect the floating-pin
interval before firmware takes control.

## Consequences

### Positive

- the light backend matches the simpler fixed-color hardware;
- brightness and transient responses remain compatible with the unchanged GUI;
- PWM generation uses timer/channel resources distinct from the display
  backlight while respecting their shared ESP32-C6 clock source;
- the R00 RGB product remains exactly recoverable from `software-r00`.

### Costs / Risks

- color/warmth and spatial effects are no longer available in R01;
- brightness percentage is a logical user value, not a direct physical duty;
- GPIO8 strap interaction, MOSFET switching behavior, thermal performance,
  audible noise and visible flicker require target verification;
- incorrect ground or dual-power wiring can produce unsafe or unintended
  behavior outside firmware control.

## Alternatives Considered

- Retain addressable RGB support alongside COB PWM — rejected for this branch
  because R01 is intentionally a small, dedicated COB-only product variant.
- Bit-bang PWM in the lighting task — rejected because hardware LEDC provides
  stable carrier timing without increasing task latency or jitter.
- Reuse display timer/channel — rejected because independent outputs and
  settings avoid resource coupling.

## Verification / Implications

- Host tests cover mapping boundaries, invalid limits, saturation and sub-floor
  fade values.
- A clean ESP-IDF v6.0 Control build shall select GPIO8, 3 kHz, 5–80% limits,
  distinct LEDC resources and application version `R01-dev`.
- Before applying 12 V, target testing shall verify GPIO8 low through reset and
  approximately 3 kHz output at expected duties with the controllers USB-powered.
- Integrated testing shall cover reset flash, brightness points, effects,
  persistence, GUI/audio regression, Audio-disconnected operation, thermal/noise
  observations and the final PD/DC-DC topology.
- No R01 release tag shall be created until the target verification gate passes.

## References

- `docs/software/02_SRS.md`
- `docs/software/04_SAS.md`
- `docs/software/05_ICD.md`
- `docs/software/06_VVM.md`
- `firmware/control/main/drivers/cob_pwm_if.c`
- `firmware/control/main/lighting/light_pwm_policy.c`
