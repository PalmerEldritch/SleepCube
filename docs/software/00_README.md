# SleepCube Software Documentation

**Baseline:** R00 (draft)

## 1. Scope

This document set specifies the SleepCube embedded software system running on two cooperating controllers:

- **Control Controller** — Waveshare ESP32-C6-Touch-LCD-1.47; owns product state, touch UI, display, lighting and coordination.
- **Audio Controller** — M5Stack AtomS3 Lite with ATOMIC Speaker Base (NS4168); owns audio storage, decoding, buffering, playback, I2S output, volume/fades and independent enforcement of finite playback duration.

The physical enclosure, production hardware design, regulatory compliance, acoustic transducer design, detailed power architecture and mechanical design are outside the active software R00 specification unless explicitly required as a software interface constraint.

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
| `../adr/*` | Architecture decision records | Normative for accepted decisions |

If documents conflict, accepted ADRs and the SRS/SAS/ICD take precedence over historical prototype documentation.

## 3. Legacy material

The former FRS/TRS/compliance matrix and P0 implementation documents remain as reference material only. Their useful behavioural requirements have been selectively carried into this software baseline. Hardware/product requirements not represented here are intentionally outside software R00.

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

## 6. Open design items

The following are intentionally not frozen by the initial R00 documentation pass:

- final audio file format(s) and encoding parameters;
- final removable/non-removable storage policy;
- exact binary/text representation of the UART protocol;
- checksum/framing implementation;
- startup/reconnection retry timing;
- exact ownership of persisted audio volume if implementation evidence favours one controller.

These shall be resolved through implementation milestones and ADRs before R00 release.