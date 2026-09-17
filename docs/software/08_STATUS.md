# SleepCube Software Development Status

**Baseline:** R00 (draft)  
**Last updated:** 2026-09-17
**Purpose:** Authoritative session-to-session handoff snapshot.

## 1. Current milestone

**M2 standalone audio service in progress**

Status: **M1 passes. M2 software and the normal 30-minute on-target hardware
gate pass on `feat/m2-standalone-audio`; 60-minute endurance playback and
content-fault injection remain pending.**

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

M2 now has an accepted content/storage policy (ADR-0005), accepted volume
persistence ownership (ADR-0006), and a clean-building standalone implementation.
The staged 1 GB card mounts on target; canonical content, silent startup,
playback/looping, fades, all volume levels, persistence and the normal finite
timer have passed. M2 must not be treated as complete until the full endurance
run and missing/unsupported-content fault injection pass.

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
- the retained M1 generated-tone source for historical diagnostic reference;
- silent startup, exact completion counters and actionable boot/fault logging;
- local build, flash, monitor and hardware-test instructions;
- a two-second silent startup-log delay so application boot/`READY` output occurs
  after the native USB Serial/JTAG endpoint can re-enumerate.

The M2 implementation additionally contains:

- integrated Speaker Base microSD mounting on GPIO6/GPIO7/GPIO8 with fixed-low
  card select and no automatic formatting;
- strict validation of `/sleepcube/audio.wav` as 44.1 kHz, mono, signed 16-bit
  RIFF/WAVE PCM;
- buffered local looping and mono-to-duplicated-stereo conversion;
- a single playback-owner task with STOPPED/STARTING/PLAYING/STOPPING/FAULT state;
- one-second start/manual-stop ramps, 250 ms volume ramps, and a five-second
  timeout fade that reaches silence at the finite deadline;
- a bounded provisional quadratic volume law capped at -12 dBFS, with Audio
  Controller NVS persistence and validation;
- normal 30-minute and separately built finite 60-minute endurance configurations;
- 60-second frame/byte/loop/deadline heartbeat logging;
- short-press PLAY/STOP and one-second-hold volume cycling as the M2-only local
  standalone test interface;
- a deterministic, loop-safe validation-WAV generator.

The failed initial M1 configuration was 44.1 kHz signed 16-bit MSB-aligned mono.
The confirmed M1 format is 44.1 kHz signed 16-bit Philips-I2S stereo
with identical left/right samples and a -24 dBFS diagnostic peak. Both use BCLK
GPIO5, LRCLK GPIO39 and DATA GPIO38. This is the confirmed hardware-output format
for M1 and remains the M2 amplifier-facing format. ADR-0005 now fixes the
separate content format as mono PCM WAV at 44.1 kHz/16-bit.

Hardware bring-up status: **PASS; build, flash, boot/reset, 21 exact-count
playbacks, final-image regression and listening gates complete.**

M2 hardware status: **Normal gate passes; endurance in progress.** The 1 GB
FAT16 card mounts through the Speaker Base fixed-low-CS SPI path and its
canonical 60-second WAV validates and loops cleanly. The controller starts and
resets silent, all four volume levels and NVS restoration pass, and the normal
1,800-second session reaches silent `STOPPED` after its five-second timeout fade.
The separately flashed endurance image boots, validates the card and reports a
3,600-second session, but endurance playback has not yet started.

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

Verified structurally for M2:

- ADR-0005 selects Speaker Base microSD and canonical 44.1 kHz/16-bit mono PCM
  WAV at `/sleepcube/audio.wav`;
- ADR-0006 assigns authoritative persisted volume to Audio Controller NVS;
- normal 30-minute and finite 60-minute endurance configurations both build
  for ESP32-S3/8 MB with 314,352-byte application images and valid image
  checksum/validation hash;
- the endurance build contains `CONFIG_SC_M2_ENDURANCE_BUILD=y`, while the
  normal build does not;
- the deterministic generator produced a valid 176,444-byte, two-second PCM WAV
  smoke-test asset with SHA-256
  `02f05343ebb010d00da938825d6046b07c488fea6a16685b69dd31294708c102`;
- `git diff --check` passes;
- build evidence and pending hardware gates are recorded in
  `docs/software/verification/2026-09-16_M2_standalone_audio.md`.

Verified for M2 card preparation:

- `/dev/sda1` was identified as a 1,006,904,320-byte FAT16 removable partition
  with UUID `A87B-A154`;
- existing root-level and trash content was inspected before staging and left
  intact;
- the generated 60-second test asset was deployed as `/sleepcube/audio.wav`;
- the on-card file is 5,292,044 bytes, is detected as 44.1 kHz/16-bit mono PCM
  WAV, and matches the generated source byte-for-byte;
- deployed SHA-256 is
  `e5be6d74956eec50828d22b66cddb85edc7d437d23e531254398755ede4ca4d4`.

Verified on M2 target hardware on 2026-09-17:

- normal and endurance images rebuilt at application version `61b3b26`, flashed
  with write verification and booted on the AtomS3 Lite;
- FAT16 card mounted with capacity 1,007,681,536 bytes and the canonical WAV
  validated as 2,646,000 frames / 60,000 ms;
- startup and reset reached silent `STOPPED`/`READY` without automatic playback;
- loop boundary, one-second start/manual-stop fades and 25/50/75/100% bounded
  volume playback were clean on the supplied 1 W/8 ohm speaker;
- persisted 75% volume was restored after reset; the saved level was returned
  to 50% before long-run testing;
- the normal 1,800-second session completed 30 loops and reached silent
  `STOPPED` with 79,381,504 frames / 317,526,016 bytes and no logged underrun,
  partial write, storage error, panic, watchdog or reboot;
- its final five-second timeout fade was reported smooth; laptop USB supplied
  power. M2-specific listening distance was not recorded;
- the separate endurance image identifies a 3,600-second session, mounts and
  validates content and reaches silent `READY`; playback remains unstarted.

Not yet verified for the new architecture:

- M2 missing-card/file and malformed/unsupported-content silent fault handling;
- Audio Controller 60-minute endurance playback and timeout behaviour;
- final normal-image restoration and short target regression after test images;
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
- ADR-0005: Speaker Base microSD with canonical 44.1 kHz/16-bit mono PCM WAV content.
- ADR-0006: Audio Controller owns authoritative persisted volume.

Historical decision:

- ADR-0001: previous Waveshare-local P0 audio pipeline; superseded by ADR-0003 and archived under `docs/legacy/adr/`.

Key ownership rule:

- Control Controller owns UI, lighting and product/session presentation state;
- Audio Controller owns actual audio execution state and audio hardware;
- audio samples are never transported between controllers.

## 5. Known issues / blockers

No architectural blocker is currently known. The M1 hardware/audio-quality risk
is closed for generated-tone and stored-PCM playback on the supplied 1 W/8 Ω
reference speaker.
The initial MSB-mono distortion was resolved by standard Philips-I2S stereo with
duplicated channels. The TEBM 10 W/4 Ω speaker was not used and is not required
for M2 entry. The available 1 GB FAT16 card has enough capacity for the selected
PCM policy; FAT mount compatibility and fixed-low-CS operation now pass on the
intended hardware.

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

- exact UART TX/RX GPIO assignment;
- protocol framing, integrity and idempotency strategy;
- final product tuning of the provisional M2 volume law and fade timing.

## 6. Next recommended task

**Run the already-flashed M2 endurance image.**

The exact next task is:

1. reconnect a no-reset serial monitor to the installed endurance image (or
   reboot and recapture its silent `READY` log if power was removed), record the
   listening distance, then short-press once and leave the unit untouched;
2. capture the complete 3,600-second run through its five-second timeout fade,
   verify monotonic heartbeats/final counters and confirm clean listening and
   silent `STOPPED` with no underrun, partial write, panic, watchdog or reboot;
3. exercise missing-card/file and malformed/unsupported-WAV startup faults,
   confirming actionable diagnostics and silent fault containment;
4. restore the normal 30-minute image, run a short card/playback regression and
   update the M2 record/VVM/status. Keep UART and Control Controller integration
   out of scope until these M2 gates pass.

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

### 2026-09-16 — M2 software baseline and storage preparation

- Created `feat/m2-standalone-audio` from the completed M1 branch.
- Accepted ADR-0005 selecting the integrated microSD card, FAT16/FAT32 and
  canonical `/sleepcube/audio.wav` 44.1 kHz/16-bit mono PCM content.
- Accepted ADR-0006 assigning authoritative persisted volume to Audio Controller
  NVS.
- Implemented storage mounting, strict WAV parsing, local buffering/looping,
  gain ramps, persistent bounded volume, finite-session timing, state/fault
  ownership and standalone button controls.
- Added a separate compile-time 60-minute endurance image without introducing a
  normal runtime path for unbounded playback.
- Added a deterministic validation-content generator and updated the Audio
  Controller operating instructions.
- Clean-built the normal and endurance configurations; both produce
  314,352-byte ESP32-S3/8 MB images with valid integrity metadata.
- On-target M2 verification has not started.
- Confirmed the available card as FAT16, preserved its existing files, deployed
  the canonical 60-second validation asset, and verified its format, size, hash
  and byte-for-byte identity after read-back.
- Paused before target flashing at the user's request. The exact resume point is
  the three-step first-target sequence in section 6.

### 2026-09-17 — M2 normal hardware gate and endurance staging

- Rebuilt the normal and endurance images at application version `61b3b26`;
  both remained 314,352 bytes for ESP32-S3/8 MB.
- Flashed the normal image and captured successful FAT16 mount, strict canonical
  WAV validation and silent deterministic `READY` on the intended AtomS3 Lite,
  Speaker Base, supplied 1 W/8 ohm speaker and laptop USB power setup.
- Verified clean standalone start/stop and content looping, all 25/50/75/100%
  volume levels, persisted-volume restoration across reset and silent startup.
- Completed the normal 1,800-second gate: 30 loops, 79,381,504 frames,
  317,526,016 bytes and a smooth five-second timeout fade to silent `STOPPED`,
  with no logged stream or runtime fault.
- Flashed and booted the separate 3,600-second endurance image. Its identity,
  saved 50% volume, card/content validation and silent `READY` state pass.
- Paused before pressing PLAY for the endurance run. No partial endurance result
  is claimed. The installed image is the endurance build and was left stopped;
  the exact resume sequence is in section 6.
- Missing/unsupported-content fault injection, the full endurance run and final
  restoration/regression of the normal image remain pending; M2 is not complete.

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
