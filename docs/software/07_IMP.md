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

**Status:** Complete. The standalone diagnostic firmware and reproducible
ESP32-S3/8 MB build configuration are implemented on
`feat/m1-audio-bringup` at source commit `838248a`. Clean-build and image-
inspection checks pass. Target
flash/hash verification passes, and one finite generated-tone sequence completed
with the expected frame/byte count. The first listening check on the supplied
1 W/8 Ω reference speaker failed due to considerable noise, clicks/pops and
apparent overdrive. A controlled Philips-I2S stereo/duplicated-channel comparison
at -24 dBFS then built, flashed and completed one exact-count sequence. Both tones
were reported clear and without distortion or clicks/pops. Explicit silence-floor
evidence also passes: the silent portions were reported quiet and without noise.
The monitor captured 21 exact-count retained-format sequences without I2S errors;
listening was at approximately 20–30 cm using laptop USB power. The final image
also passed an audio regression, complete application-log capture and ten silent
resets to `READY` without panic, watchdog or reboot loop.

A two-second silent delay before application logging makes `READY` observable
after native USB re-enumeration. The confirmed M1 format is 44.1 kHz signed
16-bit Philips-I2S stereo with duplicated channels at a -24 dBFS diagnostic peak.
See `verification/2026-09-16_M1_audio_bringup.md`.

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

**Status:** Complete on `feat/m2-standalone-audio`. The software baseline
implements the ADR-0005 microSD/PCM-WAV content path, buffered local looping,
bounded gain transitions, Audio Controller NVS volume ownership per ADR-0006,
an authoritative playback task, a normal 30-minute deadline and a separately
configured finite 60-minute endurance image. Both configurations build cleanly
for ESP32-S3/8 MB. On-target microSD/content validation, silent startup,
standalone playback/looping, all four bounded volume levels, NVS restoration and
the normal 30-minute finite timeout passed on 2026-09-17. On 2026-09-18 the
separate 60-minute endurance session completed 60 loops and reached silent
`STOPPED` without a logged stream/runtime fault or audible artifact. No-card and
unsupported-WAV startup faults remained silent with actionable diagnostics. The
canonical asset was hash-verified after restoration, and the final deployed
normal image passed a short playback regression.

A post-M2 developmental follow-up on 2026-09-18 also confirmed that the
10 W/4 ohm TEBM speaker operates cleanly at the tested 25% and 50% levels and
that the intended sleep-music track is materially better than the former
Waveshare-local path at approximately 30 cm. A faint signal-correlated hiss/buzz
remains noticeable at approximately 10 cm. Active digital silence is quiet, a
controlled 4 kHz low-pass comparison did not change the observation, and a
50%/100% comparison did not identify digital gain quantization as the primary
cause. Subsequent user listening found the noise on the supplied 1 W/8 ohm
speaker and another small 3 W/4 ohm speaker as well; it is not specific to the
TEBM. This is tracked as open issue `AUD-ISS-001`, non-blocking for M3 but
requiring explicit R00 disposition. See
`issues/AUD-ISS-001-playback-noise.md` and
`verification/2026-09-18_TEBM_music_followup.md`.

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

**Status:** Complete on `feat/m3-uart-transport`. The Waveshare
GPIO16/GPIO17 ↔ Atom Grove GPIO2/GPIO1 crossed-TX/RX mapping with common ground
and no shared 5 V is frozen in the ICD. ADR-0007 fixes bounded COBS/CRC-16
framing. Both projects include a shared codec and independent transport-only
PING/PONG task. The Waveshare M3 image retains UI/lighting but disables legacy
local audio. Host parser tests and clean ESP32-S3/ESP32-C6 builds pass.

Both targets flashed with hash verification and each matched at least 21
diagnostic PONGs. Independent resets of each controller recovered the link;
bad-CRC and truncated probes in both directions were rejected with subsequent
valid matches. The complete Atom application log showed valid card/content and
silent `STOPPED`/`READY`. The user reported the Waveshare screen normal, but no
external LED strip is connected, so physical light continuity is unverified.
A sustained peer-unavailable test also passed: while Waveshare RESET was held,
the Atom logged nine missed PONG rounds without panic/reboot, then resumed
matches after Control restarted. The physical UART jumpers were not removed.
M3's bidirectional-transfer and malformed-data exit criteria pass. The user
later confirmed speaker(s) stayed silent after the Atom reset. Physical
external-strip behavior remains unverified and is carried forward.
The first Waveshare BOOT/RESET attempt caused a recoverable USB enumeration
error `-71`; a plain RESET restored it before flashing. See
`verification/2026-09-21_M3_uart_transport.md`.

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

**Status:** Complete on `feat/m4-semantic-audio-protocol` (2026-09-23). The
semantic catalog and ADR-0008 are frozen; host tests and clean ESP32-S3/ESP32-C6
builds pass. On target, bounded PLAY/STOP/SET_VOLUME/status, exact duplicate
suppression, finite timeout, both boot orders, both independent reset directions,
silent recovery, and display/GPIO8-light coexistence pass. The USB development
console reset-recovery defect found during the gate is fixed in `3908d18`.

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

**Status (2026-09-23): IN PROGRESS.** Base implementation `1f6e326` and STOP
feedback timing correction `31d8221` clean-build; `31d8221` is flashed on the
Waveshare. Boot synchronization reconciles authoritative STOPPED/50% state with
UI and GPIO8 lighting active. Corrected STOP timing, volume and Audio-unavailable
UI/light observations remain before M5 can pass; see
`verification/2026-09-23_M5_control_audio_facade.md`.

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
- explicit `AUD-ISS-001` audio-noise disposition supported by representative
  intended-track listening, whether fixed/mitigated or accepted as a limitation;
- release candidate build instructions.

Exit criteria:

- all non-deferred SRS/ICD requirements pass;
- no unresolved R00-critical TBDs;
- `AUD-ISS-001` has an evidence-backed release disposition and is not silently
  treated as closed by UART or validation-tone success.

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

Begin M5 at the existing Control-side `audio_service` seam:

1. replace the disabled local backend with a non-blocking facade over the proven
   `sc_audio_link_control_*` client while preserving application-facing calls
   where practical;
2. route touch PLAY/STOP and volume requests to the Audio Controller, and feed
   authoritative status/events back to `app_core` and the displayed state;
3. move any audio/light cross-domain presentation coordination into `app_core`
   so lighting remains independent of UART availability;
4. verify the existing rest/adjustment UI against remote audio, then exercise
   Audio reset/unavailability without blocking display or GPIO8 lighting.

Keep M6 release fault policy and the `AUD-ISS-001` audio-noise disposition out
of the M5 facade migration. The issue's next diagnostic remains an audition of
the exact hash-matched canonical mono WAV on a known-clean reference output; it
must be fixed, mitigated or explicitly accepted before R00 release.
