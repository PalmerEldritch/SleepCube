# SleepCube Software Development Status

**Baseline:** R00 (draft)  
**Last updated:** 2026-09-11  
**Purpose:** Authoritative session-to-session handoff snapshot.

## 1. Current milestone

**M0 — Documentation baseline**

Status: **In review / essentially complete**

The software-focused R00 documentation set has been created on branch `docs/software-r00-baseline` and is under review in PR #1. No firmware behaviour has been changed as part of M0.

## 2. Current implementation state

### Control Controller — Waveshare ESP32-C6

Existing P0 firmware remains the active implementation.

Working/valuable implementation to preserve:

- touch LCD bring-up and LVGL integration;
- touch-first ambient UI;
- rest view and temporary volume/brightness adjustment view;
- RGB lighting service and lighting-effects engine;
- persistent light brightness handling;
- application event queue / `app_core` structure.

The existing local audio stack remains present in the repository but is considered legacy implementation pending migration to the Audio Controller.

Planned migration seam:

- retain the Control-side `audio_service` application abstraction where practical;
- replace its local playback/I2S backend with an inter-controller audio-link backend;
- move cross-domain audio/light coordination into `app_core` rather than allowing the audio service to directly own lighting behaviour.

### Audio Controller — AtomS3 Lite + ATOMIC Speaker Base

Implementation status: **Not yet brought up for SleepCube.**

Planned ownership:

- audio storage access;
- decoding and buffering;
- playback state machine;
- volume/gain and fades;
- I2S output to ATOMIC Speaker Base / NS4168;
- independent finite playback timeout enforcement;
- audio-link command handling and status/event reporting.

## 3. Verified state

Verified from the previous Waveshare prototype development:

- Waveshare display/touch stack is operational;
- current UI interaction model is usable and should be preserved unless requirements change;
- RGB lighting engine is operational and suitable for reuse;
- the original Waveshare-local audio path did not achieve acceptable audio quality despite substantial investigation.

Not yet verified for the new architecture:

- AtomS3 Lite + ATOMIC Speaker Base standalone audio quality;
- selected audio storage and codec path;
- Audio Controller playback/fade/timer implementation;
- UART electrical connection and GPIO assignment;
- transport framing/integrity behaviour;
- semantic protocol behaviour;
- reset, link-loss and reconciliation behaviour between controllers;
- integrated two-controller system behaviour.

## 4. Architectural decisions in force

Accepted unless superseded by a later ADR:

- ADR-0003: two-controller software architecture;
- ADR-0004: 3.3 V full-duplex UART semantic audio link and independently enforced finite Audio Controller timeout.

Key ownership rule:

- Control Controller owns UI, lighting and product/session presentation state;
- Audio Controller owns actual audio execution state and audio hardware;
- audio samples are never transported between controllers.

## 5. Known issues / blockers

No architectural blocker is currently known.

Primary technical risk is whether the purchased AtomS3 Lite + ATOMIC Speaker Base produces acceptably clean audio under the intended SleepCube operating conditions. This shall be resolved before investing in the distributed protocol implementation.

The following implementation choices remain intentionally open:

- audio codec/container and storage policy;
- exact UART TX/RX GPIO assignment;
- protocol framing, integrity and idempotency strategy;
- quantitative volume law and fade timing;
- final ownership of persisted audio volume.

## 6. Next recommended task

**M1 — Standalone Audio Controller bring-up.**

The next development session should:

1. create the Audio Controller firmware target for AtomS3 Lite;
2. confirm basic board boot/logging and toolchain configuration;
3. bring up the ATOMIC Speaker Base using its intended I2S pins;
4. play a deterministic local test signal/audio asset without any Waveshare dependency;
5. assess audible noise, distortion, transients and playback stability;
6. document exact hardware pin use and measured/observed results;
7. update this status file and the VVM/IMP before ending the session.

Do **not** begin Control↔Audio UART integration until standalone Audio Controller playback has been demonstrated sufficiently to justify continuing with the new hardware path.

## 7. Session closeout record

At the end of every development session that changes code, architecture, interfaces, verification state, milestone progress or known risks, this file shall be updated so a new session can continue from repository state alone.

The closeout update shall capture at minimum:

- current milestone and status;
- material implementation changes;
- verification performed and results;
- items not yet verified;
- known defects, blockers and open decisions;
- exact next recommended task.

Do not rely on chat history as the sole record of development state.
