# SleepCube Software Development Status

**Baseline:** R00 (draft)  
**Last updated:** 2026-09-16  
**Purpose:** Authoritative session-to-session handoff snapshot.

## 1. Current milestone

**M1 complete — M2 standalone audio service is next**

Status: **M1 passes on `feat/m1-audio-bringup`; M2 has not started.**

M0 remains complete on the underlying software R00 baseline. M1 now has a
button-triggered, finite generated-tone diagnostic and a reproducible ESP32-S3
build. The target was flashed and completed one sequence, but its initial
MSB-aligned mono output produced unacceptable noise, clicks/pops and apparent
overdrive on the supplied 1 W/8 Ω speaker. A standard Philips-I2S stereo
comparison with duplicated channels and a reduced -24 dBFS peak was then built,
flashed and produced clear tones without distortion or clicks/pops in its first
run; its silent portions were also quiet and without noise. The final image then
passed an audio regression, complete application-log capture and ten silent reset
cycles to `READY`. M1 meets its standalone hardware exit criterion.

Repository organization now reflects the two-controller architecture:

- `firmware/control/` — Waveshare ESP32-C6 Control Controller firmware;
- `firmware/audio/` — AtomS3 Lite Audio Controller ESP-IDF project and M1 diagnostic;
- `docs/software/` — active software R00 documentation;
- `docs/software/adr/` — active architecture/design decisions;
- `docs/legacy/` — superseded product/prototype documentation and historical implementation records;
- `references/` — non-normative vendor examples, pin maps, datasheets and reference source material.

## 2. Current implementation state

### Control Controller — Waveshare ESP32-C6

Existing P0 firmware has been reorganized under `firmware/control/` and remains the active control-side implementation.

Working/valuable implementation to preserve:

- touch LCD bring-up and LVGL integration;
- touch-first ambient UI;
- rest view and temporary volume/brightness adjustment view;
- RGB lighting service and lighting-effects engine;
- persistent light brightness handling;
- application event queue / `app_core` structure.

The existing local audio stack remains present in the control firmware as legacy implementation pending migration to the Audio Controller.

Planned migration seam:

- retain the Control-side `audio_service` application abstraction where practical;
- replace its local playback/I2S backend with an inter-controller audio-link backend;
- move cross-domain audio/light coordination into `app_core` rather than allowing the audio service to directly own lighting behaviour.

### Audio Controller — AtomS3 Lite + ATOMIC Speaker Base

The independent ESP-IDF project under `firmware/audio/` now contains:

- tracked ESP32-S3/8 MB/USB Serial-JTAG build defaults;
- an AtomS3 Lite board mapping for Speaker Base I2S and the local button;
- a bounded I2S output abstraction with finite write timeouts and fail-safe shutdown;
- a deterministic button-triggered 440 Hz / 1 kHz diagnostic sequence;
- silent startup, exact completion counters and actionable boot/fault logging;
- local build, flash, monitor and hardware-test instructions;
- a two-second silent startup-log delay so application boot/`READY` output occurs
  after the native USB Serial/JTAG endpoint can re-enumerate.

The failed initial configuration was 44.1 kHz signed 16-bit MSB-aligned mono.
The confirmed M1 format is 44.1 kHz signed 16-bit Philips-I2S stereo
with identical left/right samples and a -24 dBFS diagnostic peak. Both use BCLK
GPIO5, LRCLK GPIO39 and DATA GPIO38. This is the confirmed hardware-output format
for M1; M2 still owns the separate codec/container content-format decision.

Hardware bring-up status: **PASS; build, flash, boot/reset, 21 exact-count
playbacks, final-image regression and listening gates complete.**

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

Verified structurally for the R00 repository baseline:

- active software documentation is separated from legacy prototype documentation;
- Control and Audio firmware have separate project directories;
- active ADRs are separated from superseded legacy decisions;
- ADR-0001 is retained as historical decision evidence under `docs/legacy/adr/` and is superseded by ADR-0003.

Verified for M1 software/build bring-up:

- a clean out-of-tree build passes with ESP-IDF v6.0 commit
  `662a3be354759d9487bf4b1a629fadb766cb1800`;
- generated application image targets ESP32-S3 with 8 MB flash configuration;
- image checksum and validation hash are valid;
- project sources compile without warnings;
- final application image is 196,672 bytes with 81% of the 1 MB application partition free;
- the Philips-I2S stereo comparison also builds without project warnings; its
  196,672-byte ESP32-S3/8 MB image has a valid checksum and validation hash;
- the final logging-delay image remains 196,672 bytes with valid checksum and
  validation hash and passed hash-verified flash plus hardware regression;
- `git diff --check` passes;
- detailed completed evidence and the hardware procedure are recorded in
  `docs/software/verification/2026-09-16_M1_audio_bringup.md`.

Verified on the intended AtomS3 Lite target:

- `esptool` identified ESP32-S3 revision v0.2 with embedded 8 MB flash;
- bootloader, partition table and application writes passed hash verification;
- the rejected initial format completed one button-triggered sequence with
  exactly 286,650 frames / 573,300 bytes and no logged I2S transport error;
- the retained Philips-I2S image flashed with hash verification and completed
  one sequence with exactly 286,650 frames / 1,146,600 bytes and no logged write
  error; both tones were reported clear and without distortion or clicks/pops,
  and the silent portions were quiet and without noise.
- sequences 2 through 21 also completed with exactly 286,650 frames / 1,146,600
  bytes and no logged I2S timeout, partial write, panic or reboot;
- listening was performed at approximately 20–30 cm using laptop USB power.
- the final logging-delay image captured complete application identity,
  configuration, silent initialization and `READY` output;
- ten consecutive brief resets each reported reset reason 1, initialized silent
  output and reached `READY`, with no automatic tone, panic, watchdog or reboot loop;
- one final-image audio regression completed with the exact retained-format count
  and clean listening result.

Not yet verified for the new architecture:

- selected audio storage and codec path;
- Audio Controller playback/fade/timer implementation;
- UART electrical connection and GPIO assignment;
- transport framing/integrity behaviour;
- semantic protocol behaviour;
- reset, link-loss and reconciliation behaviour between controllers;
- integrated two-controller system behaviour.

## 4. Architectural decisions in force

Accepted active ADRs:

- ADR-0002: touch-first ambient UI on the Control Controller;
- ADR-0003: two-controller software architecture;
- ADR-0004: 3.3 V full-duplex UART semantic audio link and independently enforced finite Audio Controller timeout.

Historical decision:

- ADR-0001: previous Waveshare-local P0 audio pipeline; superseded by ADR-0003 and archived under `docs/legacy/adr/`.

Key ownership rule:

- Control Controller owns UI, lighting and product/session presentation state;
- Audio Controller owns actual audio execution state and audio hardware;
- audio samples are never transported between controllers.

## 5. Known issues / blockers

No architectural blocker is currently known. The M1 hardware/audio-quality risk
is closed for generated-tone playback on the supplied 1 W/8 Ω reference speaker.
The initial MSB-mono distortion was resolved by standard Philips-I2S stereo with
duplicated channels. The TEBM 10 W/4 Ω speaker was not used and is not required
for M2 entry.

The AtomS3 Lite requires manual reset-button handling around flashing: hold reset
for approximately two seconds to enter download mode before the flash command,
then press reset after flashing completes to start the application. The earlier
ROM `waiting for download` observation was caused by omitting the second manual
reset, not by a demonstrated reset defect. The application may re-enumerate under
a new device name; attach monitoring without resetting it.

The installed ESP-IDF worktree reports `v6.0-dirty` because file modes differ
from its tag, but its `HEAD` is exactly the v6.0 commit above. No changed ESP-IDF
source content was required by the SleepCube build.

The following implementation choices remain intentionally open:

- audio codec/container and storage policy;
- exact UART TX/RX GPIO assignment;
- protocol framing, integrity and idempotency strategy;
- quantitative volume law and fade timing;
- final ownership of persisted audio volume.

## 6. Next recommended task

**Begin M2 storage and codec/container selection.**

The next development session should:

1. evaluate the available Audio Controller storage path and candidate audio
   codec/container choices against memory, complexity and endurance needs;
2. record the selected content format/storage policy in the required M2 ADR;
3. implement the smallest standalone storage/read/decode/buffer path through the
   confirmed Philips-I2S output API;
4. keep UART and Control Controller integration out of this M2 entry task.

M1 now satisfies the standalone-playback prerequisite. Preserve the planned
milestone order: complete the M2 standalone audio service before beginning M3
Control↔Audio UART integration.

## 7. Session closeout record

### 2026-09-16 — M1 diagnostic implementation

- Created `feat/m1-audio-bringup` from the approved documentation baseline.
- Implemented reproducible Audio Controller defaults, board mapping, I2S output
  abstraction, generated-tone diagnostics, button trigger, bounded write/fault
  handling and project operating instructions.
- Verified clean ESP32-S3 build, 8 MB image metadata/integrity, application size
  and repository whitespace; recorded evidence in the M1 verification report.
- Subsequently flashed the image with hash verification and captured one
  successful finite diagnostic sequence on the AtomS3 Lite.
- The user confirmed that the test used the supplied 1 W/8 Ω speaker and reported
  clear tones with considerable noise, clicks/pops and apparent overdrive; the
  initial MSB-mono format therefore fails the M1 listening gate.
- Implemented and clean-built the Philips-I2S stereo comparison with duplicated
  channels and a reduced -24 dBFS peak.
- Flashed that image with hash verification and captured one exact-count
  retained-format sequence; the user reported clear tones without distortion
  or clicks/pops and quiet, noise-free silent portions.
- Captured 21 retained-format sequences in total; all completed with exact frame/
  byte counts and no logged I2S timeout, partial write, panic or reboot.
- Recorded the listening setup as approximately 20–30 cm with laptop USB power.
- A standard monitor and immediate reopen watcher both missed application startup
  logs during native USB re-enumeration. Added and clean-built a two-second silent
  startup-log delay, then flashed it with hash verification.
- Captured the final image's complete application identity/configuration/`READY`
  log and an exact-count clean audio regression.
- Verified ten consecutive silent resets to `READY` with reset reason 1 and no
  panic, watchdog, reboot loop or automatic playback.
- Corrected the operating procedure to use manual entry into download mode and
  a manual post-flash reset before attaching a no-reset monitor.
- M1 passes; this tone test does not claim storage, decoding, product-audio fades,
  session timing, endurance, UART integration or full SRS audio compliance.
- Exact next task is the four-step M2 storage/codec selection and bring-up sequence
  in section 6.

### Ongoing closeout requirement

At the end of every development session that changes code, architecture, interfaces, verification state, milestone progress or known risks, this file shall be updated so a new session can continue from repository state alone.

The closeout update shall capture at minimum:

- current milestone and status;
- material implementation changes;
- verification performed and results;
- items not yet verified;
- known defects, blockers and open decisions;
- exact next recommended task.

Do not rely on chat history as the sole record of development state.
