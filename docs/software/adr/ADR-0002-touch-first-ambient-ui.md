# ADR-0002: Touch-First Ambient UI Direction

## Status

Accepted

## Context

The original requirements set described SleepCube as a minimal bedside device with physical-button interaction and no display requirement. The validated control-side firmware and active hardware direction use the Waveshare ESP32-C6 touch LCD board as the local user-interface platform:

- the integrated LCD is part of the intended ambient presentation;
- touch input is the primary local control interface;
- a dedicated RGB LED strip remains the primary lighting output;
- the target experience is a coordinated LCD + LED ambient presentation with local touch control.

Continuing to treat the display/touch path as bring-up-only would leave requirements, implementation documentation and firmware architecture out of sync with the actual software direction.

## Decision

For the software R00 baseline:

- the Waveshare ESP32-C6 touch LCD board is the **Control Controller** and primary local user-interface platform;
- the integrated touch display is a product feature, not a temporary test aid;
- the primary user controls are:
  - audio playback toggle;
  - volume adjustment;
  - brightness adjustment;
- the primary UI shall not expose a separate light on/off control;
- the LCD and RGB LED strip shall evolve toward a coordinated ambient visual system;
- the RGB LED strip remains the dedicated lighting subsystem;
- this ADR defines UI direction only and does not assign audio execution responsibility to the Control Controller. Audio execution ownership is defined by ADR-0003.

## Consequences

### Positive

- aligns the active software requirements with the validated control hardware direction;
- preserves the existing Waveshare UI investment while allowing audio execution to move to the Audio Controller;
- enables coordinated ambient presentation across LCD and LED strip;
- keeps the local interaction model small and consistent.

### Costs / Risks

- legacy assumptions in the former product requirements and traceability documents are obsolete for software R00;
- temporary button backends remain debug-only rather than product-defining;
- final LCD/LED synchronization still requires further implementation and verification work;
- UI presentation must represent remote audio state correctly once the two-controller integration is implemented.

## Alternatives Considered

- keep physical buttons as the primary product interface and treat LCD/touch as prototype-only;
- allow both touch and light-toggle-heavy physical control as co-equal product models;
- treat the LCD only as a settings/control surface and exclude it from ambient presentation.

## References

- `docs/software/03_UX.md`
- `docs/software/04_SAS.md`
- `docs/software/adr/ADR-0003-two-controller-software-architecture.md`
- `docs/legacy/01_FRS.md`
- `docs/legacy/02_TRS.md`
- `docs/legacy/implementation/SC_P0_RuntimeServices.md`
- `docs/legacy/implementation/SC_P0_LightingEngine.md`
