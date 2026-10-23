# ADR-0010 — Retain a nonzero minimum brightness in the normal GUI

**Status:** Accepted

**Date:** 2026-10-24

**Supersedes:** Normal-GUI off-selection clause of ADR-0009

**Superseded by:** None

## Context

ADR-0009 defined internal logical 0% as fully off and initially described it as
a user setting. During R01 GPIO8 verification, the retained GUI was confirmed to
bottom out at the active lighting minimum rather than expose an off selection.
The product decision is to retain that behavior for now so the R01 migration
does not introduce a new GUI state. A user-accessible off position may be
reconsidered later.

## Decision

- The normal brightness GUI shall retain a nonzero minimum selection and shall
  not expose lighting off.
- The lowest active COB output shall remain bounded by the configured 5% PWM
  duty floor. Ambient fluctuation may raise it, but shall not drive it below the
  floor after startup has settled.
- Internal logical 0%, driver initialization, reset and fail-safe paths shall
  continue to command 0% duty and suppress effects.
- The GUI layout, gestures, persistence behavior and controller ownership shall
  remain unchanged.
- Adding a user-accessible off selection later requires an explicit UX/software
  decision and renewed verification.

## Consequences

### Positive

- R01 preserves the established GUI behavior while changing only the lighting
  hardware backend.
- The active strip cannot be accidentally turned fully off through normal
  brightness adjustment.
- Zero-duty safety behavior remains available independently of the GUI.

### Costs / Risks

- A user cannot turn the COB strip fully off through the normal brightness GUI.
- Internal off behavior cannot be physically selected through the production
  GUI and therefore relies on host mapping tests plus reset/fail-safe evidence.
- The distinction between GUI minimum and internal off must remain explicit in
  requirements and verification records.

## Alternatives Considered

- Make the bottom of the existing slider turn the strip off — deferred because
  it would change the retained GUI behavior during the hardware migration.
- Add a separate off control — rejected for R01 because it would change the GUI
  layout and interaction model.

## Verification / Implications

- Target verification shall measure the active GUI minimum at or above the 5%
  duty floor, plus midpoint and maximum behavior.
- GPIO8 shall be verified at 0 V during reset before 12 V is applied.
- Host policy tests shall retain explicit 0% mapping and invalid-configuration
  fail-off coverage.
- A settled 0% target measurement through the normal GUI is not applicable.

## References

- `docs/software/02_SRS.md`
- `docs/software/03_UX.md`
- `docs/software/06_VVM.md`
- `docs/software/adr/ADR-0009-cob-pwm-lighting.md`
- `firmware/control/main/services/light_service.c`
- `firmware/control/main/lighting/light_pwm_policy.c`
