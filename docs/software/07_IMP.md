# SleepCube Software Implementation Plan

**Revision:** R01 development

## 1. Development strategy

Development shall proceed in bounded milestones. Each milestone must preserve a buildable repository and produce evidence appropriate to the milestone before the next architectural dependency is added.

The released R00 Waveshare GUI and application-facing lighting behavior are
retained. M10 replaces only the physical RGB backend and RGB-specific rendering
with fixed-color COB PWM. The Audio firmware and UART protocol remain unchanged.

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

**Status (2026-09-23): COMPLETE.** Base implementation `1f6e326` and STOP
feedback timing correction `31d8221` clean-build; `31d8221` is flashed on the
Waveshare. Boot synchronization reconciles authoritative STOPPED state with UI
and GPIO8 lighting active, the corrected STOP response is confirmed at fade
onset, remote volume follows adjustment without artifacts, and linked brightness
control is smooth across the display and GPIO8 strip. Audio-unavailable UI/light
containment also passes: Atom reset stopped output, unavailable audio input
showed no false PLAY state, and brightness remained responsive. Post-restart
reconciliation to a new Audio session now also passes without replay. The
post-recovery PLAY/STOP regression confirms clean audio and synchronized visual
feedback. Both M5 exit criteria pass; see
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

**Status (2026-09-23): PASS.** Implementation `709512c` adds periodic bounded
GET_STATUS health probes, neutral/unavailable presentation after link timeout,
configurable finite product PLAY duration and bounded persisted-setting repair.
The reproducible timer overlay is tracked separately from the 1,800-second
product defaults, and final diagnostic-label correction `9e28ab7` makes both
durations explicit in logs. Strict tests and clean builds pass. Physical
timer-expiry, UART-loss/recovery, Control-reset deadline survival, distributed
power-cycle persistence and final product restore all pass; see
`verification/2026-09-23_M6_session_fault_integration.md`.

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

**Status (2026-09-23): PASS.** Implementation `727285d` prevents a
moved/cancelled rest-view gesture from falling through as a short-tap audio
toggle, adds focused timing markers, and provides separate product and trace
profiles. Dark-room testing then identified unstable strip chromaticity below
the empirically clean 26% selected level and during the low-brightness STOP
dip. Correction `f776eb6` adds LED-only 26% selected and 17% post-effect floors
while retaining the display's complete range and the stored user value.

Strict host regressions and clean ESP32-C6/8 MB product/trace builds pass.
Physical short tap, moved-gesture suppression, 1,020 ms long press,
approximately three-second return, direct controls, confirmed-state response,
dark-room behavior and Audio-unavailable/recovery checks pass. The corrected
strip stays warm white at minimum output and through the STOP dip; low-output
animation coarseness is accepted as a hardware-resolution characteristic. The
exact product image was restored with trace disabled and confirmed silent with
normal display/LED operation. Evidence is in
`verification/2026-09-23_M7_ux_lighting.md`.

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

**Status (2026-09-26): PASS.** The requirement/ICD audit is complete and
maps all 55 SRS, 24 UX and 29 ICD IDs. Five previously omitted SRS rows were
added to the VVM, stale deferred-design text was resolved, and the validated
audio transition durations are now explicit R00 criteria. M8 tooling adds exact
Control request/response timestamps, a tested timing-log analyzer, a tracked
3,600-second distributed-endurance overlay and pre-playback truncated-WAV
rejection. Product, trace, endurance and Audio profiles clean-build. Corrected
target timing passes with 20 samples (0.453 ms maximum touch acceptance and
59.710 ms maximum user-command acknowledgement), and five independent resets
of each controller pass, including active-playback cases. Reset testing exposed
and corrected a product-enabled M4 diagnostic console at `ac421a7`; the
corrected regression shows no host command injection or volume mutation.
Missing-file and truncated-WAV boots now fail silent with actionable diagnostics
while UI/lighting remain responsive; the canonical asset is hash-restored. The
no-cross-board-5-V inspection passes, and `AUD-ISS-001` is explicitly accepted
as a documented R00 limitation after exact-content reference listening.
The integrated 3,600-second session reached timer-expired `STOPPED` with
`fault=0` and no user-observed audible/visual artifact. The exact product image
was restored and three coordinated dual-controller power cycles retained
silent startup, no automatic PLAY, normal display/strip behavior and persisted
brightness/volume. All M8 exit criteria pass. See
`verification/2026-09-23_M8_system_validation.md`.

Post-M8 R00 adjustment (completed 2026-09-27): the user approved changing the normal
product-session default from 1,800 to 3,600 seconds. The protocol range and all
ownership/fault semantics remain unchanged, and M8-END-001 already passes the
complete 3,600-second distributed path. Exact `3348213` product artifacts then
passed silent synchronized startup, a normal PLAY reporting 3,600 seconds on
both controllers, and a clean manual fade to silent `STOPPED` with `fault=0`
and no automatic restart. `R00-DUR-001` passes; see
`verification/2026-09-26_R00_60min_session_default.md`.

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

**Status (2026-09-27): PASS.** The active document/ADR audit found no
R00-critical behavioral TBD, and the accepted `AUD-ISS-001` limitation remains
explicit. The superseded Waveshare-local audio implementation, configuration,
Helix dependency and SPIFFS image generation were removed from the active
Control tree. A tracked `sdkconfig.r00.defaults` now defines the release
Control profile, and both firmwares embed stable version `R00`.

Strict host regressions and exact ESP-IDF v6.0 ESP32-C6/ESP32-S3 builds pass.
The exact images flashed with hash verification and passed silent synchronized
startup, finite 3,600-second PLAY establishment, clean manual fade to silent
`STOPPED`, `remaining=0`, `fault=0`, and normal display/strip observation. The
active documents are promoted to R00 and annotated tag `software-r00`
identifies the release. Evidence is in
`verification/2026-09-27_M9_R00_release.md`.

Deliverables:

- documentation promoted from draft to R00;
- final ADR status review;
- implementation documentation updated to match released code;
- legacy P0 audio code clearly archived/removed from active build path;
- tagged release.

### M10 — R01 COB PWM lighting migration

**Goal:** Replace the R00 addressable RGB backend with the fixed-color 12 V COB
hardware while preserving the GUI, persistence, audio behavior and controller
ownership boundaries.

**Status (2026-10-25): SOFTWARE COMPLETE; UNPOWERED TARGET GATE PASS; POWERED
PROTOTYPE PARTIAL; PWM DEBUG SERIAL SWEEP PASS.** Branch
`feat/r01-cob-pwm-lighting` was created from clean tag `software-r00`. The
Control firmware now uses an active-high GPIO8 LEDC driver at 3 kHz/13-bit on
timer 1/channel 1, retains the `light_service` API and whole-strip effects, maps
internal logical 0% to off and nonzero values to configurable 5–80% steady-state
duty, and fails off for invalid limits or output errors. ADR-0010 retains the
existing nonzero GUI minimum; no user-accessible off state is added. RGB
buffers, per-pixel rendering, RMT sources/configuration and RGB-only settings
are removed from the active Control implementation. The GUI, Audio firmware and
UART protocol are unchanged.

Strict mapping/policy tests, retained host regressions and a clean ESP-IDF v6.0
R01 Control build pass. The image embeds `R01-dev`; exact software evidence is
recorded in `verification/2026-10-19_M10_cob_pwm_implementation.md`.

The powered prototype subsequently reported stable 12 V and 5 V rails with
both controllers, Audio and the COB strip functioning and smooth brightness
control. A development-only USB console now accepts 1–25 kHz carrier selection
in 1 kHz steps, fixed 0–100% duty override, status and return-to-automatic
commands. Host parser tests and a clean build pass; the exact image also passes
target flash and serial runtime checks at 1, 9, 10, 19, 20
and 25 kHz while holding fixed 42.12% duty. Independent scope measurements of
frequency and duty remain to be recorded.

Deliverables:

- COB LEDC driver and host-tested brightness/duty policy;
- R01 GPIO8/3 kHz/5–80% product configuration;
- ADR-0009 and active specification/verification updates;
- development-topology GPIO measurements before applying 12 V;
- powered-strip behavioral and regression checks;
- final PD-trigger/DC-DC integrated power-cycle smoke.
- development-only runtime PWM frequency/duty console for hardware evaluation.

Exit criteria:

- GPIO8 initializes low and measures approximately 3 kHz at expected duties;
- reset/boot produces no unintended COB flash;
- reset-low, GUI minimum, midpoint, maximum, fades, ambient movement and PLAY/STOP pulses
  pass without visible flicker, excessive MOSFET heating or objectionable noise;
- unchanged GUI, persistence, UART/audio and Audio-unavailable behavior pass;
- final USB-disconnected PD/DC-DC power-cycle smoke passes;
- exact evidence is recorded before any R01 release tag is created.

## 3. Repository layout

```text
firmware/
├── control/                 # Waveshare ESP32-C6 firmware
├── audio/                   # AtomS3 Lite firmware
└── shared/
    └── protocol/            # wire-level types/constants if safely shareable
```

M0 established this two-controller layout, M9 released it unchanged, and M10
changes only the Control-side lighting implementation within it.

## 4. Development constraints

- Preserve working Waveshare GUI and application-facing lighting behavior unless a requirement explicitly changes it.
- Do not implement low-level remote filesystem/decoder control in the Control Controller.
- Do not allow PLAY without a finite Audio Controller deadline.
- Do not make normal lighting behaviour depend on UART availability.
- Do not prematurely freeze codec/framing choices before corresponding milestone evidence.
- Any architecture change affecting ownership or the ICD requires an ADR update before being treated as baseline.

## 5. Suggested immediate next engineering task

Execute M10-HWT-001 with both controllers USB-powered and the external 12 V
supply initially off. Confirm the external GPIO8 gate pull-down and explicit
external-source-to-Waveshare ground connection, flash the exact R01 image, then
measure reset/boot low level, approximately 3 kHz frequency and expected duties.
Only after this passes should 12 V be applied for the COB behavior checks.
