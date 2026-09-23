# SleepCube Software Development Status

**Baseline:** R00 (draft)  
**Last updated:** 2026-09-23

**Purpose:** Authoritative session-to-session handoff snapshot.

## 1. Current milestone

**M5 Control audio-facade migration in progress; physical UI/fault gate next**

Status: **M1 through M4 pass. M5 implementation commit `1f6e326` replaces the
disabled local Control audio backend with a non-blocking semantic-link facade,
routes touch/confirmed state through `app_core`, and moves audio/light
coordination out of `audio_service`. Strict inherited host tests, the retained
local-backend compatibility build and clean ESP32-C6/8 MB builds pass. Initial
touch evidence exposed STOP feedback delayed until the one-second fade ended;
`31d8221` now triggers presentation on authoritative STOPPING while retaining
the pending request until STOPPED. That exact corrected image is flashed and
its boot log shows UI/light startup plus authoritative STOPPED/50%/no-fault
reconciliation without PLAY. The user confirmed PLAY feedback synchronized to
the tap, corrected STOP feedback synchronized to fade onset, and remote volume
following adjustment without artifacts. Brightness behavior is smooth and
synchronized across the display and GPIO8 strip. The
Audio-unavailable containment gate also passes: Atom reset stopped output, an
audio tap produced no false PLAY state, and brightness remained responsive.
Atom restart was silent; Control negotiated new Audio session `508b7be2` and
reconciled through GET_STATUS to STOPPED/51%/no fault without replay. One final
post-recovery PLAY/STOP regression remains before M5 can pass.**

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
playback/looping, fades, all volume levels, persistence, normal finite timer,
60-minute endurance run and representative missing-storage/unsupported-content
fault injection pass. The M2 validation asset remains preserved on the card and
the normal image is the final deployed firmware. A post-M2 follow-up also
confirms clean electrical operation with the 10 W/4 ohm TEBM speaker and
materially improved sleep-music quality at the intended approximately 30 cm
listening distance. The active card content is now the full converted
Interstellar mono track. A signal-correlated music-playback noise is now tracked
as open issue `AUD-ISS-001`: it was heard on three different speakers, remains
non-blocking for M3, and needs explicit R00 disposition.

M3 now has a frozen ICD mapping: Waveshare GPIO16 TX to Atom GPIO2 RX, Atom
GPIO1 TX to Waveshare GPIO17 RX, and common ground with no 5 V inter-controller
connection. ADR-0007 specifies bounded COBS/CRC-16 framing. Both firmwares now
build with a shared codec and a UART1 diagnostic PING/PONG task. The user
confirmed the jumpers are connected and both boards have separate USB power,
and bidirectional UART communication has now been verified by matched PONGs.
The no-5-V hookup has not been independently inspected. Both M3 images flashed
with hash verification on 2026-09-21. The Atom's initial transport task logged
expected missing PONGs while the Waveshare still ran old firmware; after the
Waveshare flash, both sides matched at least 21 responses. See
`verification/2026-09-21_M3_uart_transport.md`.

Later directed fault probes, independent resets and a sustained Control-reset
interval also passed. The Atom logged nine missed PONG rounds, stayed running,
and resumed matches after Waveshare restart. These are M3 transport-only
results, not M4 PLAY/STOP/volume or release-level UI/light evidence.
The latest Atom boot reported `STOPPED` with persisted volume 100%; M3 never
issued PLAY, but a future local button PLAY would use that saved level unless
changed. The user later confirmed that the connected speaker(s) stayed silent
after reset. The external LED strip was connected after M3, so no M3 light
observation is retroactively claimed.

After the Waveshare BOOT/RESET operation, its previously enumerated USB device
`AC:EB:E6:1E:67:C4` disappeared; the host logged failed re-enumeration with
error `-71`. No flash command was run at that point. The user's subsequent
RESET restored the stable USB identity, and the M3 Control image then flashed
with all write hashes verified. Both boards now exchange and match diagnostic
PING/PONG. The Control storage partition was not flashed.

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

The existing local audio stack remains present only as a legacy compatibility
backend; the normal M5 configuration selects the Audio Controller facade.

For M4, `sdkconfig.m4.defaults` selects the 8 MB ESP32-C6/USB Serial-JTAG build
with UI and lighting enabled and legacy local audio disabled. A dedicated UART
owner performs HELLO/GET_STATUS synchronization, correlated semantic requests,
bounded retries and reset-session detection. A development USB console can
issue bounded remote commands without yet connecting the product touch UI.

M5 `sdkconfig.m5.defaults` retains that hardware configuration and selects the
remote `audio_service` facade. Rest-view and volume UI requests now enter
`app_core`, then use the semantic client. The facade exposes only reconciled
state, always requests a finite 1,800-second session, suppresses duplicate
playback intent and coalesces volume drag traffic. Confirmed state returns to
the UI and drives any audio-related light response through `app_core`; light
startup and brightness remain independent of audio synchronization.

Implemented migration seam:

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

M4 replaces the M3-only diagnostic with a semantic UART1 endpoint on Atom GPIO1
TX/GPIO2 RX. It starts before storage/audio initialization and reports
NOT_READY or startup FAULT until the playback owner becomes available. Validated
finite PLAY, STOP and volume requests are queued to that sole owner; synchronized
snapshots report state, volume, remaining time, stop reason and fault code.
Shared framing and semantic code live under `firmware/shared/protocol/`; the
single UART client/endpoint implementation is under `firmware/shared/transport/`.

The failed initial M1 configuration was 44.1 kHz signed 16-bit MSB-aligned mono.
The confirmed M1 format is 44.1 kHz signed 16-bit Philips-I2S stereo
with identical left/right samples and a -24 dBFS diagnostic peak. Both use BCLK
GPIO5, LRCLK GPIO39 and DATA GPIO38. This is the confirmed hardware-output format
for M1 and remains the M2 amplifier-facing format. ADR-0005 now fixes the
separate content format as mono PCM WAV at 44.1 kHz/16-bit.

Hardware bring-up status: **PASS; build, flash, boot/reset, 21 exact-count
playbacks, final-image regression and listening gates complete.**

M2 hardware status: **PASS.** The 1 GB
FAT16 card mounts through the Speaker Base fixed-low-CS SPI path and its
canonical 60-second WAV validates and loops cleanly. The controller starts and
resets silent, all four volume levels and NVS restoration pass, and the normal
1,800-second session reaches silent `STOPPED` after its five-second timeout fade.
The separate 3,600-second endurance session completed 60 loops and reached
silent `STOPPED` without a logged stream/runtime fault or reported audible
artifact. Representative absent-card and unsupported-WAV faults remained silent
with actionable diagnostics. The normal image was restored and regressed.

Post-M2 speaker/music follow-up status: **PASS WITH OPEN ISSUE `AUD-ISS-001`.**
The TEBM28C10-4/A 10 W/4 ohm speaker produced clean low-frequency validation content
at 25% and 50%, with no distortion or detected warmth. The full Interstellar
mono test asset was clear, fuller and not overdriven at approximately 30 cm,
with no click, pop or dropout. A faint hiss/buzz-like component was noticeable
only at close range (approximately 10 cm) and stopped with music. Active digital
silence was quiet; a 4 kHz low-pass A/B and 50%/100% gain comparison did not
isolate it to high-frequency speaker response or primarily to digital gain
quantization. See
`docs/software/verification/2026-09-18_TEBM_music_followup.md`.

On 2026-09-21 the user also heard the noise with the supplied 1 W/8 ohm speaker
and another small 3 W/4 ohm speaker, making a TEBM-only defect unlikely. A
headphone-jack attempt produced severe high-pitched noise and is not a valid
reference test because that jack shares the NS4168 differential Class-D output.
Do not repeat it with those headphones. Detailed open issue and safe follow-up:
`docs/software/issues/AUD-ISS-001-playback-noise.md`.

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

Verified on M2 target hardware on 2026-09-17 and 2026-09-18:

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
  power;
- the separate 3,600-second endurance session completed all 60 loops at
  approximately 30 cm, ending in `STOPPED` with 158,761,472 frames /
  635,045,888 bytes, no logged stream/runtime fault and no audible artifact;
- absent-card startup produced actionable `ESP_ERR_TIMEOUT`, while a valid but
  unsupported 8-bit WAV produced actionable `ESP_ERR_NOT_SUPPORTED`; both
  disabled playback and produced no sound;
- the canonical 16-bit asset was restored and its original SHA-256 reverified;
  the normal image was rebuilt/flashed at version `082c7ed` and passed silent
  boot plus a clean 349,184-frame / 1,396,736-byte PLAY/STOP regression.

Verified in the 2026-09-18 post-M2 TEBM/music follow-up:

- the TEBM28C10-4/A 10 W/4 ohm speaker remained clear at 25% and 50% during
  low-frequency validation playback, with no distortion or detected warmth;
- the converted Interstellar PCM track was clear and materially better than the
  previous Waveshare-local path at approximately 30 cm, with good bass fullness
  and no overdrive, click, pop or dropout;
- a faint signal-correlated hiss/buzz was barely audible at 30 cm but noticeable
  at approximately 10 cm and stopped with playback;
- active digital silence was quiet, a controlled 4 kHz low-pass comparison did
  not change the noise, and increasing firmware volume from 50% to 100% did not
  materially improve its ratio to the music;
- subsequent user listening found the noise on the TEBM, supplied 1 W/8 ohm
  speaker and another small 3 W/4 ohm speaker; the two smaller speakers made
  the noise more prominent relative to the music. This is subjective evidence,
  not a matched-level or logged comparison;
- the full music asset and retained diagnostic/M2 assets were hash-verified on
  the card, volume was restored to persisted 50%, and the card was safely
  unmounted. Full evidence is in
  `docs/software/verification/2026-09-18_TEBM_music_followup.md`.

Verified for M3 software preparation on 2026-09-21:

- the shared codec host test passes with strict compiler warnings and UBSan,
  covering maximum-payload round trip, malformed COBS/version/length/CRC,
  incomplete/overlong frames, timeout and recovery;
- clean ESP-IDF v6.0 ESP32-S3/8 MB Audio and ESP32-C6/8 MB Control builds pass
  from external build/configuration paths, followed by passing incremental
  rebuilds after the final transport-task change;
- the Control M3 configuration has USB Serial/JTAG console, UI and lighting
  enabled and legacy local audio disabled; the Audio M3 configuration retains
  the M2 standalone playback path;
- final pre-flash application images are 338,656 bytes (Audio) and 595,200
  bytes (Control); no on-target result is claimed from these builds;
- both USB Serial/JTAG devices enumerate, with the Atom matched to its prior
  stable USB ID; the user confirmed the jumper hookup and bidirectional traffic
  verified the UART signal path. The absence of a cross-board power rail was
  not independently inspected.
- the Atom M3 image from source commit `5894e14` was flashed to ESP32-S3 v0.2
  with embedded 8 MB flash; bootloader, partition table and application writes
  passed hash verification. After manual reset the user reported application
  startup. A no-reset monitor caught periodic M3 PINGs and missing PONGs as
  expected with the unflashed Waveshare; the complete boot log and silent/card
  validation were captured on the later independent Atom reset below.
- after RESET recovered its USB port, the Waveshare M3 image from source commit
  `86d8e3e` flashed to ESP32-C6FH8 v0.2/8 MB with verified bootloader,
  partition table and application hashes;
- independent no-reset USB monitors captured at least 21 matched PONGs and
  peer PING echoes on each board. Basic crossed-pin bidirectional UART transfer
  works; the first malformed/truncated probes occurred before this capture,
  but both directed probes were later captured during independent resets.
- the user reported the Waveshare screen normal while M3 transport was active.
  No external LED strip is connected, so external-light behavior is not
  verified. Later monitor counters reached at least 118 on both boards but did
  not show a user-triggered reset or fault-probe sequence.
- a later independent Waveshare RESET was captured: the Control diagnostic
  sequence restarted at 1, both boards resumed matched PONGs, and the
  still-running Atom rejected Control's bad-CRC and time-expired partial
  probes before matching subsequent valid replies. No Atom panic/reboot was
  observed;
- a subsequent independent Atom RESET was captured with complete application
  identity/configuration, valid card/content, auto-clear I2S and silent
  `STOPPED`/`READY` logs. The still-running Waveshare rejected the Atom's
  bad-CRC and truncated probes, then matched subsequent valid PONGs. No
  Control panic/reboot was observed. Atom restored persisted volume 100%. The
  user later confirmed the connected speaker(s) stayed silent after reset.
- during a sustained Waveshare RESET hold, the Atom logged nine missed PONG
  rounds, rejected malformed restart bytes, did not panic/reboot, and resumed
  matched replies. Control reinitialized its display and matched replies after
  restart. No UART jumper was physically disconnected.

Verified for M4 software preparation on 2026-09-23:

- the semantic ICD type/payload/status/result catalog and ADR-0008 session,
  idempotency and bounded retry strategy are frozen;
- strict host tests pass for HELLO/INFO, status, finite PLAY, STOP, volume,
  malformed/unsupported input, duplicate suppression, stale/reused IDs,
  session mismatch, reset reconciliation and timer-expiry events;
- clean ESP-IDF v6.0 builds pass for Audio ESP32-S3/8 MB and the UI/light-safe
  Control ESP32-C6/8 MB configuration, with 341,568-byte and 600,064-byte
  preparation images respectively;
- the implementation preserves one Audio playback owner and one UART owner per
  controller; UI/light tasks do not perform UART waits, and no audio samples or
  low-level Audio operations cross the protocol;
- deployed Audio embeds `94c7b4a`, is 341,568 bytes and has SHA-256
  `5cf5a22a2abbbe6605de9a2e8e09732c827f157a6f1880247ba81988b31c3de3`;
  final Control embeds console-recovery fix `3908d18`, is 600,144 bytes and has
  SHA-256 `353472dc9b1065abbf5d2ad314eb5b10021b70f9a7069c6c6828c67a2e277697`;
- detailed scope, commands and hardware evidence are in
  `docs/software/verification/2026-09-23_M4_semantic_audio_protocol.md`.

Verified on M4 target hardware on 2026-09-23:

- Atom booted silent, mounted/validated canonical content and reached READY;
  Control synchronized with HELLO/INFO/GET_STATUS and issued no automatic PLAY;
- remote PLAY, STOP, 25/50/75/100% volume, GET_STATUS and authoritative events
  passed with no command timeout or playback fault;
- an exact duplicate PLAY returned DUPLICATE while remaining time continued to
  fall, and the single session ended at its original deadline through a smooth
  five-second fade and TIMER_EXPIRED;
- explicit STOP produced a clean one-second fade; the user reported no click,
  pop, dropout or other transition artifact;
- resetting Control during PLAY did not interrupt, restart or extend Audio;
  restarted Control used HELLO/GET_STATUS and did not replay PLAY;
- resetting Audio during PLAY stopped output safely; Control detected the new
  Audio session, abandoned old command context and reconciled to silent STOPPED
  without its own reset or an automatic PLAY;
- Audio-first and Control-first startup both pass; display and GPIO8 LED strip
  remained operational through command and reset activity;
- a sticky stdio EOF found in the USB development harness after Control reset
  was fixed with `clearerr(stdin)` in `3908d18`; attaching only after boot and
  issuing GET_STATUS then passed.

Not yet verified for the new architecture:

- exhaustive SRS-AUD-009 permutations beyond the representative M2 absent-card
  and unsupported-format fault cases;
- a complete early Control boot log after native USB re-enumeration; later
  transport startup/recovery was proven from both controller logs;
- M5 touch UI/audio-service integration and Audio-absent UI/light behavior
  through the product facade.

## 4. Architectural decisions in force

Accepted active ADRs:

- ADR-0002: touch-first ambient UI on the Control Controller;
- ADR-0003: two-controller software architecture;
- ADR-0004: 3.3 V full-duplex UART semantic audio link and independently enforced finite Audio Controller timeout.
- ADR-0005: Speaker Base microSD with canonical 44.1 kHz/16-bit mono PCM WAV content.
- ADR-0006: Audio Controller owns authoritative persisted volume.
- ADR-0007: bounded COBS/CRC-16 UART framing, 16-bit sequence correlation and
  M3-only diagnostic PING/PONG types.
- ADR-0008: per-boot semantic sessions, state-changing command IDs, Audio-side
  duplicate suppression, bounded retries and status-only recovery.

Historical decision:

- ADR-0001: previous Waveshare-local P0 audio pipeline; superseded by ADR-0003 and archived under `docs/legacy/adr/`.

Key ownership rule:

- Control Controller owns UI, lighting and product/session presentation state;
- Audio Controller owns actual audio execution state and audio hardware;
- audio samples are never transported between controllers.

## 5. Known issues / blockers

No architectural blocker is currently known. The M1 hardware/audio-quality risk
is closed for generated-tone and M2 validation-PCM playback on the supplied
1 W/8 Ω reference speaker; this does not close the sleep-music issue below.
The initial MSB-mono distortion was resolved by standard Philips-I2S stereo with
duplicated channels. The TEBM 10 W/4 Ω speaker is now demonstrated compatible
at the tested levels and substantially improves the intended music relative to
the original path. Open issue `AUD-ISS-001` records a signal-correlated hiss/buzz
that was also heard with the supplied 1 W/8 Ω speaker and another small 3 W/4 Ω
speaker. With the TEBM it was noticeable at approximately 10 cm. It is not
present during active digital silence and was not changed by removing content
above 4 kHz or by a 12 dB digital-gain comparison. A headphone-jack comparison
was invalid because the jack directly shares the filterless differential
Class-D output; severe high-pitched noise occurred and that test should not be
repeated with those headphones. The exact mono asset has not yet been confirmed
clean or noisy on a known-clean reference output. The root cause is unresolved;
the issue does not block M3 but requires an explicit fix, mitigation or accepted
limitation before R00 release. The available 1 GB FAT16 card has
enough capacity for the selected PCM policy; FAT mount compatibility and
fixed-low-CS operation pass on the intended hardware.

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

- independent physical inspection of the no-cross-board-power-rail hookup;
- final product tuning of the provisional M2 volume law and fade timing;
- root cause and release disposition of `AUD-ISS-001`, including exact-mono
  reference audition and representative listening after any mitigation.

Observed M3 setup issue: the Waveshare USB Serial/JTAG endpoint failed to
enumerate after manual BOOT/RESET (kernel error `-71`), then returned after a
plain RESET. The ESP-IDF automatic-reset flash succeeded. This did not indicate
an M3 UART or firmware failure, but boot/download-mode handling should be
watched in future flashes. M4 confirmed that the Waveshare can enter download
mode automatically for flashing; no manual BOOT/RESET preparation is required.
Do not hot-unplug one controller while its UART lines are connected to the
powered other board.

## 6. Next recommended task

**Complete the M5 physical UI and Audio-unavailable gate.**

The exact next task is:

1. Confirm silent startup and normal display/GPIO8 light presentation on the
   currently running `31d8221` image.
2. Run one post-recovery PLAY/STOP cycle and confirm clean audio plus
   synchronized authoritative visual feedback.
3. Record the result in
   `verification/2026-09-23_M5_control_audio_facade.md`; only then mark M5 pass
   and make M6 the next task. Keep `AUD-ISS-001` outside the M5 claim.

## 7. Session closeout record

### 2026-09-23 — M5 software, flash and boot preparation

- Created `feat/m5-control-audio-facade` from the completed M4 baseline and
  committed the facade/UI migration as `1f6e326`.
- Added tracked M5 defaults selecting ESP32-C6/8 MB, UI/light and remote audio
  while disabling the legacy local audio backend.
- Implemented finite PLAY, authoritative state getters, duplicate playback
  suppression, coalesced volume requests and fail-safe clearing of stale intent.
- Routed touch and confirmed-state events through `app_core`; removed direct
  audio-service-to-light-service ownership and speculative tap feedback.
- Strict host framing/semantic tests pass. Both M5 remote and retained legacy
  Control configurations build. Initial touch evidence showed STOP feedback
  waited for the one-second fade to finish; logs isolated this to presentation
  treating STOPPING as active.
- Added correction `31d8221`: STOPPING now triggers confirmed stop presentation,
  but the pending STOP remains held until STOPPED to reject a tap during fade.
  Its clean image is 603,152 bytes with SHA-256
  `0ca8ebb10880ab66a9926870da082460826ae6cee5e3098da2e68fe676b7e324`.
- Confirmed USB identities, flashed only the Waveshare with hash verification,
  and captured final app `31d8221` boot through STOPPED/50% authoritative
  reconciliation. User retest confirms PLAY feedback at tap and corrected STOP
  feedback at fade onset. Remote volume changes are confirmed in Audio logs and
  the user heard the selected loudness follow without artifacts. Brightness and
  the GPIO8 strip change smoothly and in sync. Atom reset stopped active output;
  unavailable audio input produced no false PLAY response and brightness stayed
  responsive. Atom restarted silent, and Control reconciled to new Audio session
  `508b7be2` at STOPPED/51%/no fault without replay. One final PLAY/STOP
  regression remains.
- Exact next action is the four-step physical gate in section 6 and the M5
  verification record. Do not claim M5 complete yet.

### 2026-09-23 — M4 semantic protocol software preparation

- Created `feat/m4-semantic-audio-protocol` from the completed M3 baseline.
- Froze message IDs, bounded payloads, states/results, status snapshot and
  finite 1–3,600 second PLAY domain in the ICD; accepted ADR-0008 for boot
  sessions, command idempotency, one outstanding request, 250 ms/three-attempt
  retry and reconciliation without automatic replay.
- Added the shared semantic codec/server and replaced the M3 PING/PONG task
  with a single role-specific Audio endpoint or Control client using the same
  UART/parser boundary.
- Extended the Audio playback-owner queue with explicit finite PLAY, STOP,
  volume and synchronized status snapshots. Storage, decode and I2S operations
  remain private to Audio.
- Added the UI/light-safe M4 Control defaults and a development USB console;
  existing touch audio behavior is intentionally not migrated before M5.
- Strict host framing and semantic tests pass. Clean ESP-IDF v6.0 builds pass
  for ESP32-S3/8 MB and ESP32-C6/8 MB with no observed project-code warning.
- Committed the implementation as `66f36e5`, rebuilt both targets cleanly so
  that version is embedded, and recorded exact final image sizes and SHA-256
  hashes in the M4 verification report.
- No image has been flashed and no M4 hardware behavior is claimed. Exact next
  task is the five-step hardware gate in section 6. The connected LED strip and
  `AUD-ISS-001` remain explicit evidence items.

### 2026-09-23 — M4 two-controller hardware completion

- Flashed Audio and Control with hash verification; Atom required manual
  download-mode/post-flash reset, while Waveshare automatic bootloader entry
  worked and its settings partition was preserved.
- Verified silent Audio READY, HELLO/INFO/GET_STATUS, remote PLAY/STOP, all four
  volume levels, status/events, exact duplicate suppression and the original
  finite timeout. User listening checks found clean start/manual-stop/timeout
  fades and silent stopped output.
- Verified Control reset during playback without interruption, replay or
  deadline extension, and Audio reset during playback with safe stop, new
  session detection and status-only reconciliation. Both boot orders pass.
- Found the M4 USB development console retained stdio EOF if the monitor was
  absent during Control boot. Added `clearerr(stdin)`, clean-built and deployed
  fix `3908d18`, then verified post-boot monitor attach and GET_STATUS.
- User confirmed the Waveshare display and GPIO8 LED strip stayed operational
  through the command/reset tests. No panic, watchdog, playback fault or UART
  command timeout was observed.
- M4 passes. Exact next task is the five-step M5 facade/UI migration in section
  6. `AUD-ISS-001` remains open and is not closed by protocol success.

### 2026-09-21 — M3 framing and transport software preparation

- Froze the crossed UART GPIO/header mapping in the ICD and replaced the stale
  Waveshare GPIO7/GPIO8 candidate in the SAS.
- Accepted ADR-0007 for bounded COBS/zero-delimited packets with CRC-16,
  sequence correlation, parser recovery and transport-only PING/PONG types.
- Added a shared codec, host tests, independent UART1 diagnostic task on each
  controller, and one deliberate bad-CRC/partial-packet probe per direction
  after the link has matched three PONGs. No remote audio command is present.
- Added the Waveshare M3 build defaults that retain UI/light but disable the
  old local audio backend; M2 Atom playback remains standalone.
- Host tests and both 8 MB target builds pass. User confirmed three UART
  jumpers and separate USB power; USB IDs were mapped. Neither M3 image has
  yet been boot-verified, and no physical PING/PONG, fault-injection, reset or
  UI/light coexistence test has yet been performed. `AUD-ISS-001` remains open.
- Subsequently flashed the Atom image from commit `5894e14` after user download-
  mode go-ahead. ESP32-S3 v0.2/8 MB identity and all write hashes verified.
  The user performed the manual reset and reported application start. The
  no-reset monitor captured ongoing M3 PINGs and expected missing PONGs while
  the Waveshare remained on old firmware, but not a complete boot log.
- Exact next task: prompt the user for Waveshare download mode and go-ahead,
  flash/monitor its M3 image, then complete the M3 hardware gate in section 6.
- The user then entered Waveshare download mode and gave flash go-ahead, but
  its USB endpoint disappeared and failed re-enumeration with kernel error
  `-71`. No Waveshare flash command ran. The exact next action is a Waveshare
  RESET press without unplugging either board, then a fresh USB identity check.
- After user RESET, the Waveshare stable USB endpoint returned. Its M3 image
  flashed with verified ESP32-C6FH8 v0.2/8 MB identity and all write hashes.
  Both no-reset monitors captured at least 21 matched PONGs per controller
  and peer PING echoes. Invalid-frame probes ran before monitoring and are
  not yet direct on-target evidence; nor are complete boot logs, UI/light
  continuity or reset/link-loss behavior. The exact next task is the four-step
  hardware evidence sequence in section 6.
- The user reported the Waveshare screen normal, but no external LED strip is
  connected. The active monitors showed continued PONG matches, not a fresh
  reset. Ask explicitly for a Waveshare RESET press while monitoring, then
  capture the startup and fault probes; do not infer a reset from the visual
  report.
- The user then pressed Waveshare RESET. Its diagnostic sequence restarted,
  the Atom remained up, and transport recovered. Waveshare's deliberate bad-CRC
  and timed-out partial probes were rejected by the Atom; subsequent PONGs
  matched. The exact next task is an independent Atom reset with both monitors
  attached, to verify the reciprocal probes and full Audio boot/card log.
- The user then reset the Atom while Waveshare stayed up. The complete delayed
  Atom application log showed valid FAT mount/content and silent
  `STOPPED`/`READY` startup. Control rejected Audio's bad-CRC and truncated
  probes and resumed matched PONGs. Both directed fault-injection tests pass.
  The exact next task is a sustained peer-unavailable test by holding
  Waveshare RESET about eight seconds, then confirming recovery without
  touching UART jumpers. External LED-strip behavior remains unverified.
- The user held Waveshare RESET while Atom stayed powered. The Atom logged nine
  missed PONG rounds and rejected malformed restart bytes without panic or
  reboot, then resumed matches after Control restarted. The Control log showed
  display initialization and resumed PONG matches. No jumper was removed.
  M3's transport exit criteria pass; external LED-strip output was not
  observed. Exact next task is M4 semantic ICD/idempotency/status design in
  section 6.
- In a follow-up clarification, the user confirmed the connected speaker(s)
  were silent after the Atom reset. This closes the M3 subjective silent-start
  observation only; `AUD-ISS-001` and external LED-strip validation remain open.

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

### 2026-09-18 — M2 endurance, fault injection and completion

- Recaptured the endurance image's silent boot at persisted 50% volume and
  recorded the listening distance as approximately 30 cm.
- Completed the full 3,600-second endurance session: 60 loops, 158,761,472
  frames, 635,045,888 bytes and finite timeout to `STOPPED`, with no logged
  underrun, partial write, storage error, panic, watchdog or reboot. No audible
  artifact was reported.
- Verified absent-card startup containment: actionable `ESP_ERR_TIMEOUT`, no
  automatic retry/reboot, playback disabled and no sound.
- Preserved the canonical asset, temporarily staged a valid one-second 8-bit
  WAV, and verified actionable `ESP_ERR_NOT_SUPPORTED`, disabled playback and no
  sound. Restored and reverified the original asset hash afterward.
- Rebuilt and hash-verified the normal 30-minute image at version `082c7ed`,
  flashed it as the final deployed firmware, and captured restored-card silent
  `READY` plus a clean 349,184-frame / 1,396,736-byte PLAY/STOP regression.
- M2 passes. No UART, Control Controller integration, final product audio tuning
  or exhaustive release-level content-fault claim is made. Section 6 defines
  the exact M3 entry task.

### 2026-09-18 — M3 preparation and TEBM/music follow-up

- Created `feat/m3-uart-transport` from the completed M2 baseline.
- Inventoried exposed and already-used pins. The current conflict-free M3
  candidate is Waveshare GPIO16 TX to Atom GPIO2 RX, Atom GPIO1 TX to Waveshare
  GPIO17 RX, plus common ground and no inter-controller 5 V connection. Ordered
  jumper cables are not yet available, so the mapping is not electrically
  verified or frozen in the ICD.
- Connected the TEBM28C10-4/A 10 W/4 ohm speaker to the Speaker Base and verified
  silent reset, clean validation-tone output at 25%/50%, clean stop transitions,
  no distortion and no detected warmth.
- Converted the unfiltered Interstellar stereo WAV to the canonical mono PCM
  format without normalization. At approximately 30 cm, the user reported
  appropriate sleep-music loudness, good clarity and bass fullness, no overdrive,
  click, pop or dropout, and a definite improvement over the former hardware
  path.
- Investigated a faint hiss/buzz noticeable at approximately 10 cm. It stops
  with music; active digital silence is quiet; a 4 kHz low-pass A/B produced no
  noticeable change; and a 50%/100% gain comparison did not materially improve
  the noise-to-music ratio. The source remains unresolved and is non-blocking for
  M3.
- Restored the full Interstellar mono asset as `/sleepcube/audio.wav`, retained
  both the A/B diagnostic and original M2 validation asset, hash-verified all
  copies, restored persisted volume to 50%, synced and safely unmounted the card.
- Added `verification/2026-09-18_TEBM_music_followup.md` and VVM developmental
  evidence `M2-FUP-001`. No firmware source, stable architecture or interface
  contract changed.
- Exact next task: freeze the M3 electrical contract and protocol framing ADR,
  update the SAS/ICD, then implement the parser/serializer harness while awaiting
  the physical cables.

### 2026-09-21 — Audio-noise issue registration

- User compared the Interstellar track through the TEBM 10 W/4 ohm, supplied
  1 W/8 ohm, and another small 3 W/4 ohm speaker. The noise was present with all
  three and more prominent relative to music on the two smaller speakers; the
  TEBM retained the best overall sound. Exact conditions for the two additional
  speaker swaps were not recorded, and no new serial capture was made.
- Headphones in the Speaker Base's 3.5 mm jack produced severe high-pitched
  noise with music nearly obscured. The jack shares the NS4168 differential
  Class-D speaker output, so this was not an independent reference path. Do not
  repeat that test with those headphones. No damage was established.
- Created open issue `AUD-ISS-001` with reproduction context, evidence,
  exclusions, safe next diagnostics and an explicit R00 disposition requirement.
  Updated the follow-up verification record, VVM and implementation plan so
  successful M3 work cannot silently close the audio issue.
- No firmware, architecture or ICD contract changed; M3 remains in preparation.
- Exact next M3 task remains the physical/framing design freeze in section 6.
  The separate audio follow-up is to audition the exact hash-matched canonical
  mono WAV on a known-clean reference output, then select targeted diagnostics
  from `docs/software/issues/AUD-ISS-001-playback-noise.md`.

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
