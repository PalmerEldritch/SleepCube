# ADR-0002: Touch-First Ambient UI Direction

## Status

Accepted

## Context

The original requirements set described SleepCube as a minimal bedside device with
physical-button interaction and no display requirement. The validated P0 firmware and
active hardware direction have now moved to the Waveshare ESP32-C6 touch LCD board:

- the integrated LCD is part of the intended ambient presentation
- touch input is the primary local control interface
- a dedicated RGB LED strip remains the primary lighting output
- the target experience is a coordinated LCD + LED ambient presentation with local touch control

Continuing to treat the display/touch path as bring-up-only would leave requirements,
implementation docs, and firmware architecture out of sync with the actual product direction.

## Decision

Adopt the following product direction for Rev 1:

- the Waveshare ESP32-C6 touch LCD board is the primary product platform for current firmware work
- the integrated touch display is a product feature, not a temporary test aid
- the primary user controls are:
  - audio playback toggle
  - volume adjustment
  - brightness adjustment
- the primary UI shall not expose a separate light on/off control
- the LCD and RGB LED strip shall evolve toward a coordinated ambient visual system
- the RGB LED strip remains the dedicated lighting subsystem

## Consequences

### Positive

- aligns requirements with the validated hardware direction
- lets the firmware architecture converge around a real target instead of a placeholder UI model
- enables coordinated ambient presentation across LCD and LED strip
- simplifies product interaction around a small, consistent control surface

### Negative

- legacy assumptions in Rev 0 requirements and traceability documents become obsolete
- temporary button backends must be treated as debug-only instead of product-defining
- final LCD/LED synchronization still requires further implementation work

## Alternatives Considered

- keep physical buttons as the primary product interface and treat LCD/touch as prototype-only
- allow both touch and light-toggle-heavy physical control as co-equal product models
- treat the LCD only as a settings/control surface and exclude it from ambient presentation

## References

- `docs/01_FRS.md`
- `docs/02_TRS.md`
- `docs/implementation/SC_P0_RuntimeServices.md`
- `docs/implementation/SC_P0_LightingEngine.md`
