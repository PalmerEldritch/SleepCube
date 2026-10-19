# SleepCube Software Documentation

**Baseline:** R01 development (derived from released `software-r00`)

## 1. Scope

This document set specifies the SleepCube embedded software system running on two cooperating controllers:

- **Control Controller** — Waveshare ESP32-C6-Touch-LCD-1.47; owns product state, touch UI, display, lighting and coordination.
- **Audio Controller** — M5Stack AtomS3 Lite with ATOMIC Speaker Base (NS4168); owns audio storage, decoding, buffering, playback, I2S output, volume/fades and independent enforcement of finite playback duration.

The physical enclosure, production hardware design, regulatory compliance, acoustic transducer design, detailed power architecture and mechanical design are outside the active software specification unless explicitly required as a software interface constraint. R01 adds only the documented COB-light electrical/software boundary and development/final power arrangements.

## 2. Document authority

| Document | Purpose | Normative? |
| --- | --- | --- |
| `01_PRD.md` | Product/software intent and user-visible outcomes | Yes, high level |
| `02_SRS.md` | Software requirements | Yes |
| `03_UX.md` | User interaction and visual behaviour | Yes where stated as requirements |
| `04_SAS.md` | Software architecture and ownership boundaries | Yes |
| `05_ICD.md` | Control↔audio communication contract | Yes |
| `06_VVM.md` | Requirement verification plan | Yes |
| `07_IMP.md` | Implementation milestones and release gates | Process baseline |
| `08_STATUS.md` | Authoritative current development state and session-to-session handoff | Process record |
| `adr/*` | Architecture decision records | Normative for accepted decisions |
| `issues/*` | Tracked unresolved defects/risks, follow-up tests and disposition criteria | Development record |

If documents conflict, accepted ADRs and the SRS/SAS/ICD take precedence over historical prototype documentation.

`08_STATUS.md` does not redefine requirements or architecture. It records the latest implementation/verification state, known issues and next recommended task so development can continue without dependence on chat history.

## 3. Legacy material

The former FRS/TRS/compliance matrix, P0 implementation documents and superseded architecture decisions remain under `../legacy/` as reference material only. Their useful behavioural requirements have been selectively carried into this software baseline. Hardware/product requirements not represented here are intentionally outside the active software baseline.

Vendor examples, pin maps, datasheets and other non-normative technical source material are maintained under the repository-level `/references/` directory.

## 4. Requirement IDs

Use stable IDs:

- `PR-<AREA>-###` — product/software outcomes
- `SRS-<AREA>-###` — software requirements
- `UX-<AREA>-###` — user-experience requirements
- `ICD-<AREA>-###` — interface requirements

Areas include `SYS`, `UI`, `LGT`, `AUD`, `SES`, `COM`, `SET`, `FLT`, `STA`.

## 5. Terminology

| Term | Meaning |
| --- | --- |
| Control Controller | Waveshare ESP32-C6 board executing main product logic/UI/lighting firmware |
| Audio Controller | AtomS3 Lite executing audio firmware on the ATOMIC Speaker Base |
| Audio link | Local wired UART connection between the two controllers |
| Session | One finite user-initiated audio playback period |
| Rest view | Default ambient UI state shown during normal operation |
| Adjustment view | Temporary UI used for volume and brightness adjustment |

## 6. Session continuity

At the start of a development session, read `08_STATUS.md` before making implementation changes.

At the end of any session that materially changes implementation, architecture, interfaces, verification state, milestone progress, known defects or technical risk, update `08_STATUS.md` before ending the session. The handoff shall be sufficient for a fresh development session to continue from repository state alone.

Detailed closeout rules are defined in the repository `AGENTS.md`.

## 7. Released baseline and active variant

R00 remains frozen at annotated tag `software-r00`. R01 development is isolated
on `feat/r01-cob-pwm-lighting`; it replaces only the Control-side addressable
RGB implementation with fixed-color COB PWM control. The GUI, Audio firmware,
semantic UART protocol and established controller ownership remain unchanged.

ADR-0009 defines the R01 lighting/power decision and supersedes only the
RGB-specific portions of ADR-0002 and ADR-0003.

## 8. Resolved R00 design items and remaining release issue

The design items left open after M2 are now resolved. ADR-0007 freezes binary
COBS/CRC framing, ADR-0008 freezes semantic sessions and bounded retry/recovery,
and the SRS freezes the M1–M7-validated audio transition durations. ADR-0005
continues to define the R00 content/storage policy and ADR-0006 assigns
authoritative persisted volume to the Audio Controller.

The bounded quadratic R00 gain curve and transition implementation are recorded
in the Audio Controller README. Any later product retuning shall be treated as
an intentional verified behavior change, not an unresolved R00 placeholder.

`AUD-ISS-001` is the explicit audio-quality limitation. M8 representative and
exact-content listening supports accepting it for R00; its physical root cause
remains a documented post-R00 improvement item.
