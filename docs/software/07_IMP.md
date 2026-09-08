# SleepCube Software Implementation Plan

**Revision:** R00 draft

## 1. Development strategy

Development shall proceed in bounded milestones. Each milestone must preserve a buildable repository and produce evidence appropriate to the milestone before the next architectural dependency is added.

The existing Waveshare UI and lighting implementation is treated as retained functionality. The existing local audio implementation is treated as legacy/reference material and shall be removed or disabled only after replacement behaviour is demonstrated on the Audio Controller.

## 2. Milestones

### M0 — Documentation and repository baseline

**Goal:** Establish the software-focused R00 development contract.

Deliverables:

- PRD, SRS, UX, SAS, ICD, VVM and IMP active documents;
- ADRs for agreed architecture decisions;
- updated `AGENTS.md` documentation rules;
- legacy FRS/TRS/prototype documents clearly marked non-authoritative for software R00;
- proposed target repository structure documented.

Exit criteria:

- architecture boundaries are internally consistent;
- unresolved choices are explicit TBDs rather than accidental implementation assumptions.

### M1 — AtomS3 / Speaker Base hardware bring-up

**Goal:** Prove basic target hardware control independently of SleepCube integration.

Deliverables:

- new Audio Controller ESP-IDF project;
- deterministic boot/log output;
- confirmed I2S pin/configuration for ATOMIC Speaker Base;
- basic generated-tone or known-sample playback;
- initial storage bring-up if storage hardware is used at this stage.

Exit criteria:

- clean, repeatable audio output on reference 8-ohm speaker;
- no dependency on Waveshare controller.

### M2 — Standalone audio service

**Goal:** Make AtomS3 an autonomous finite-session audio appliance.

Deliverables:

- content storage/access;
- chosen codec/container implementation;
- decoder/buffer pipeline;
- start/stop fades;
- volume control;
- 30-minute independent session timer;
- authoritative playback state machine;
- audio-related fault handling.

Required ADRs:

- audio content format/storage policy;
- volume persistence ownership if resolved here.

Exit criteria:

- standalone PLAY/STOP/volume test interface works;
- finite timer cannot be bypassed through normal playback API;
- 60-minute playback endurance run passes or documented blocker exists before moving forward.

### M3 — UART transport bring-up

**Goal:** Establish robust bidirectional controller communication independent of product semantics.

Deliverables:

- UART drivers on GPIO chosen for both boards;
- framing parser/serializer;
- malformed/truncated frame handling;
- protocol logging/test harness;
- final physical pin mapping.

Required ADR:

- protocol framing/integrity strategy.

Exit criteria:

- bidirectional messages transfer reliably;
- injected malformed data does not destabilize either controller.

### M4 — Semantic audio protocol

**Goal:** Implement the normative ICD contract.

Deliverables:

- HELLO/GET_INFO;
- GET_STATUS;
- PLAY with finite duration;
- STOP;
- SET_VOLUME;
- READY/STATUS/PLAYING/STOPPED/TIMER_EXPIRED/ERROR semantics;
- acknowledgement/result correlation strategy;
- startup synchronization and independent reboot handling.

Exit criteria:

- automated/integration test coverage for all non-deferred ICD requirements;
- both boot orders and independent resets verified.

### M5 — Waveshare audio-service migration

**Goal:** Replace local audio backend while preserving existing UI/application API where practical.

Deliverables:

- `audio_link` transport/protocol client;
- `audio_service` refactored into control-side facade;
- `app_core` coordination updated for authoritative remote audio state;
- local P0 audio path disabled from normal Waveshare build;
- existing UI volume/playback interactions connected to remote state.

Refactoring requirement:

- remove direct audio-service → light-service coordination; move cross-domain effects to `app_core` or product-state coordination layer.

Exit criteria:

- existing rest/adjustment UI controls remote audio;
- lighting continues independently with Audio Controller absent.

### M6 — Session and fault integration

**Goal:** Close system-level safety/recovery requirements.

Deliverables:

- timer-expiry state reconciliation;
- UART timeout/degraded-state handling;
- Audio Controller reset recovery;
- Control Controller reset-during-playback verification;
- malformed-message and link-loss fault injection;
- persisted volume/brightness behaviour finalized.

Exit criteria:

- SRS-FLT and SRS-SES requirements pass VVM tests.

### M7 — UX and lighting integration refinement

**Goal:** Preserve/refine the existing mature UI/light behaviour against the new distributed state model.

Deliverables:

- confirmed vs requested audio-state presentation;
- unavailable/unknown audio behaviour;
- audio state visual pulse coordination through app/product state;
- slider state synchronization;
- dark-room manual UX pass.

Exit criteria:

- all non-deferred UX requirements pass.

### M8 — System validation

**Goal:** Produce full R00 evidence.

Deliverables:

- complete VVM execution;
- 60-minute audio endurance test;
- repeated independent controller reset tests;
- link disconnect/reconnect test;
- power-cycle persistence tests;
- latency/timing evidence;
- release candidate build instructions.

Exit criteria:

- all non-deferred SRS/ICD requirements pass;
- no unresolved R00-critical TBDs.

### M9 — R00 release

Deliverables:

- documentation promoted from draft to R00;
- final ADR status review;
- implementation documentation updated to match released code;
- legacy P0 audio code clearly archived/removed from active build path;
- tagged release.

## 3. Proposed repository target

```text
firmware/
├── control/                 # Waveshare ESP32-C6 firmware
├── audio/                   # AtomS3 Lite firmware
└── shared/
    └── protocol/            # wire-level types/constants if safely shareable
```

The physical repository move should occur only when it simplifies M1-M5 work. M0 shall not rename the existing firmware tree merely for cosmetic consistency.

## 4. Development constraints

- Do not rewrite working Waveshare UI/lighting code without a demonstrated requirement.
- Do not implement low-level remote filesystem/decoder control in the Control Controller.
- Do not allow PLAY without a finite Audio Controller deadline.
- Do not make normal lighting behaviour depend on UART availability.
- Do not prematurely freeze codec/framing choices before corresponding milestone evidence.
- Any architecture change affecting ownership or the ICD requires an ADR update before being treated as baseline.

## 5. Suggested immediate next engineering task

After M0 documentation approval, M1 should begin with the AtomS3 Lite + ATOMIC Speaker Base as a completely standalone target. The first proof should be a generated or known PCM tone/sample through the NS4168 before storage/codec/protocol complexity is introduced.