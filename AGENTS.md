# SleepCube agent instructions

## Scope

This repository contains embedded firmware and engineering documentation for the SleepCube software system.

The **active software specification baseline** is in `/docs/software/` and defines software behaviour, architecture, interfaces, verification and implementation milestones for the two-controller system.

Earlier product/prototype FRS, TRS, compliance, hardware and P0 implementation documents remain reference/history unless explicitly referenced by an active software document or accepted ADR.

## Active document hierarchy

- `/docs/software/01_PRD.md` — high-level product/software outcomes
- `/docs/software/02_SRS.md` — normative software requirements
- `/docs/software/03_UX.md` — user interaction and display behaviour
- `/docs/software/04_SAS.md` — software architecture and ownership
- `/docs/software/05_ICD.md` — Control↔Audio interface contract
- `/docs/software/06_VVM.md` — verification and validation matrix
- `/docs/software/07_IMP.md` — implementation/release plan
- `/docs/adr/` — accepted architecture/design decisions

If active documents conflict with legacy prototype documents, do not silently resolve the conflict from legacy material. Follow accepted ADRs and the active SRS/SAS/ICD, or flag the conflict/TBD.

## Writing rules for requirements

- Use Markdown tables for normative requirement sets where practical.
- Use one requirement per row.
- Use `shall` for normative requirements.
- Keep PRD requirements focused on user/product outcomes rather than implementation detail.
- Put software behaviour and measurable constraints in SRS.
- Put ownership, dependencies, state models and subsystem boundaries in SAS.
- Put wire/protocol/electrical interface contracts in ICD.
- Do not invent hardware, acoustic, regulatory or mechanical requirements that are outside the active software scope.
- Never invent standards clauses or compliance references. If unknown, write `TBD` and identify the missing input.

## IDs and traceability

Use stable IDs defined by `/docs/software/00_README.md`:

- `PR-<AREA>-###`
- `SRS-<AREA>-###`
- `UX-<AREA>-###`
- `ICD-<AREA>-###`

Every normative SRS requirement should reference one or more PR requirements in `Derived From` where practical.

VVM shall cover all non-deferred normative SRS requirements before R00 release.

Do not renumber existing IDs solely to make tables visually sequential.

## Architecture constraints

Unless superseded by an accepted ADR:

- Waveshare ESP32-C6 is the Control Controller and owns UI, display, lighting and overall product/session presentation state.
- AtomS3 Lite + ATOMIC Speaker Base is the Audio Controller and owns storage access, decoding, buffering, gain/fades, I2S and authoritative playback state.
- Inter-controller control/status uses a local full-duplex UART semantic protocol.
- Audio samples are not transported between controllers.
- Normal PLAY shall always establish a finite Audio Controller timeout.
- Lighting shall not depend on Audio Controller availability.
- Preserve working Waveshare UI/lighting behaviour unless a requirement explicitly justifies change.

## Implementation discipline

- Work against the current milestone in `/docs/software/07_IMP.md`.
- Keep each controller's state ownership explicit.
- Prefer deterministic state machines/event queues over distributed shared state.
- Do not make UI/light tasks block indefinitely on UART/audio operations.
- Do not expose low-level Audio Controller filesystem/decoder/I2S operations through the normal Control Controller application API.
- Update the ICD before treating protocol changes as stable.
- Add/update an ADR for architectural decisions or changes in ownership/interface strategy.
- Add verification evidence/coverage when implementing a normative requirement.

## Documentation promotion trigger

Trigger phrase: `promote: <scope>`

Meaning: functionality is mature enough for implementation documentation to be updated from observed/validated behaviour.

Required actions:

- update relevant implementation/reference documentation;
- update `/docs/software/06_VVM.md` when verification evidence or acceptance criteria change;
- update `/docs/software/07_IMP.md` milestone status/notes when applicable;
- update public API comments if contracts changed;
- add/update ADR when an architectural decision was introduced or changed;
- update `/docs/software/05_ICD.md` for stable inter-controller contract changes.
