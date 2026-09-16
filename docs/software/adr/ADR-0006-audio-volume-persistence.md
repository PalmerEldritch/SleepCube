# ADR-0006 — Audio Controller volume persistence ownership

**Status:** Accepted

**Date:** 2026-09-16

**Supersedes:** None

**Superseded by:** None

## Context

The SRS requires audio volume to survive normal power cycles. The Audio
Controller owns gain application and authoritative playback state, while the
future Control Controller UI will request volume changes over the semantic
audio protocol. Persisting the authoritative value in both controllers would
create ambiguity after independent reset or stale-state recovery.

## Decision

The Audio Controller shall own authoritative persisted audio volume for R00.

- the semantic volume domain shall be an integer percentage from 0 through 100;
- the Audio Controller shall validate and clamp a persisted value before use;
- an accepted volume change shall be stored in the Audio Controller's NVS;
- unchanged values shall not cause another NVS write;
- gain changes during playback shall use a bounded ramp rather than a discrete
  coefficient step;
- the Control Controller may cache requested/displayed volume for UX purposes,
  but shall reconcile it with Audio Controller state after startup or reset;
- playback state and remaining session time shall not be persisted or restored.

The exact perceptual volume curve and amplifier-facing maximum gain remain
implementation parameters subject to M2 listening evidence. Changing those
parameters does not change persistence ownership.

## Consequences

### Positive

- leaves one authoritative persisted value next to the subsystem that applies it;
- independent Control Controller reset cannot overwrite volume implicitly;
- startup validation and safe defaults remain local to audio hardware;
- avoids unnecessary flash wear when a value is unchanged.

### Costs / Risks

- the future protocol must report/reconcile authoritative volume;
- the Control UI may briefly show unknown volume until synchronization;
- NVS failure must reject a requested persistent change or report a fault rather
  than pretending it was saved.

## Alternatives Considered

- Persist only on the Control Controller — rejected because the Audio Controller
  can boot and operate independently and owns actual gain.
- Persist on both controllers — rejected because independent resets can produce
  conflicting values and unclear reconciliation rules.
- Do not persist volume — rejected because it violates `SRS-SET-002`.

## Verification / Implications

- M2 shall change volume, reset/power-cycle the Audio Controller and confirm the
  selected value is restored and validated;
- M4 protocol work shall include authoritative volume in status/reconciliation;
- fault injection shall cover absent/invalid NVS values without maximum-volume
  or automatic-playback behaviour.

## References

- `docs/software/02_SRS.md`
- `docs/software/04_SAS.md`
- `docs/software/adr/ADR-0003-two-controller-software-architecture.md`
- `firmware/audio/main/audio_settings.c`
