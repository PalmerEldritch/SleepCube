# SleepCube Software Development Status

**Baseline:** R01 development from released `software-r00`

**Last updated:** 2026-10-31

**Purpose:** Authoritative session-to-session handoff snapshot.

## 1. Current milestone

**M10 R01 COB PWM migration: matched touch-debug images flashed and boot-
verified; target touch/PWM/audio interaction verification pending**

Status: **The COB baseline branch was created from clean annotated tag
`software-r00`; current branch `feat/r01-touch-debug-ui` is a separate
development image. The COB-only Control implementation and active R01 documents
are complete. New mapping tests and retained settings/protocol/timing tests pass.
A clean ESP-IDF v6.0 ESP32-C6/8 MB build passes and embeds `R01-dev`. The exact
image has now flashed to the USB-powered Waveshare with write-hash verification,
and reset boot/init plus UART reconciliation pass with 12 V disconnected. GPIO8
scope measurements confirm stable 0–3.3 V PWM at 3.012 kHz plus responsive
ambient and PLAY/STOP duty effects, the 5% active floor and 0 V at reset.
ADR-0010 retains the nonzero GUI minimum, so a GUI-selected settled-off capture
is not applicable. The externally powered prototype now reports stable 12 V/5 V
rails, functioning controllers/audio/COB output and smooth brightness. A newer
development image adds runtime USB-console frequency and fixed-duty controls.
The exact clean-built image is flashed and serial runtime checks pass across
the 1–25 kHz range and both resolution boundaries. Independent scope checks at
the selected frequencies remain open. The new touch-debug Control/Audio pair
adds a five-tap 2-by-4 hardware-test grid, exact 0–100% duty, 1–25 kHz
frequency, generated C3–C8 sine source and source switching. Strict protocol/
tone host tests and clean ESP-IDF v6.0 matched builds pass. The exact matched
pair has now passed target write/hash verification and boot smoke testing.
Control booted `R01-touch-debug`, initialized GPIO8 at requested
3 kHz/actual 3001 Hz and synchronized with capability mask `0x000f`. Audio
booted `R01-touch-debug-audio`, mounted the SD card, validated the canonical WAV
and reached silent `STOPPED` at persisted 50% volume. Interactive touch-grid,
generated-tone and powered PWM checks remain open. M10 and R01 release are not
complete. No R01 release tag exists or should be created yet.**

The Control backend now drives an active-high logic-level MOSFET on GPIO8 using
LEDC timer 1/channel 1 at 3 kHz and 13-bit resolution, distinct from display
timer 0/channel 0. Internal logical brightness 0% is off and suppresses effects;
nonzero values map to configurable 5–80% steady-state duty. The normal GUI
retains its nonzero minimum and does not expose off. Startup/internal shutdown
fades may pass below 5%; ambient and PLAY/STOP effects remain capped at 80%.
Invalid floor/cap configuration and PWM update failure are fail-off paths. RGB
buffers, color/warmth/dithering, LED count/order, spatial animation and project
RMT sources/dependencies are absent from the active Control implementation.

The R01 development profile now includes a volatile COB PWM USB console. It can
select 1–25 kHz in integer 1 kHz steps, hold a requested 0–100% duty independent
of GUI/effects, report actual frequency/resolution, and return to automatic
lighting. The driver retains 13-bit resolution through 9 kHz, 12 bits through
19 kHz and 11 bits through 25 kHz. This diagnostic deliberately bypasses normal
product duty limits and must be disabled before a release image.

User terminal testing confirmed commands work when Enter sends CR+LF, but the
earlier image did not accept CR-only termination and its one-second
`audio_link` info traffic cluttered the terminal. The follow-up image accepts
CR, LF and CR+LF and reduces `audio_link` logging to warnings/errors while the
PWM console is enabled. It is now flashed and target-verified: each line-ending
form executed one command, and routine `audio_link` info traffic remained
suppressed.

The display and COB outputs retain distinct LEDC timers/channels but share the
ESP32-C6 LEDC global source. Both are pinned to the 80 MHz PLL-divided clock so
runtime COB selection through 25 kHz cannot conflict with the display timer.

The normal GUI, persistence API and controller ownership remain unchanged.
ADR-0011 records the separate touch-debug profiles and their optional semantic
source/note extension; normal profiles compile out the grid and generator.
ADR-0009 records the COB PWM architecture and the
separate development/final power arrangements; ADR-0010 records the retained
nonzero GUI minimum. Historical R00 target evidence remains valid only for R00
and is preserved below and in the existing reports.

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
Interstellar mono track. A signal-correlated music-playback noise is tracked as
`AUD-ISS-001`: it was heard on three different speakers and its physical cause
remains unresolved. M8 exact-content/reference listening now accepts it as a
documented low-level R00 limitation rather than a release blocker.

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
- `docs/software/` — active R01 development documentation, with R00 reports retained;
- `docs/software/adr/` — active architecture/design decisions;
- `docs/legacy/` — superseded product/prototype documentation and historical implementation records;
- `references/` — non-normative vendor examples, pin maps, datasheets and reference source material.

## 2. Current implementation state

### Control Controller — Waveshare ESP32-C6

The R01 Control firmware remains under `firmware/control/`.

Working/valuable implementation to preserve:

- touch LCD bring-up and LVGL integration;
- touch-first ambient UI;
- rest view and temporary volume/brightness adjustment view;
- unchanged `light_service` API with COB duty effects;
- persistent light brightness handling;
- application event queue / `app_core` structure.

The superseded Control-local storage/decoder/player/I2S implementation is no
longer present in the active source or build graph. Its history remains in Git.
The Control `audio_service` has one backend: the Audio Controller semantic
facade.

M10 replaces the old RMT/addressable-strip layer with `cob_pwm_if` plus the
host-testable `light_pwm_policy`. `sdkconfig.r01.defaults` selects GPIO8, 3 kHz,
5–80% duty, normal UI/audio behavior, `R01-dev` and the development PWM console.
The addressable-strip encoder/driver and RGB lighting engine are removed on this
branch; recover them from `software-r00` if R00 behavior is needed.

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

M6 adds periodic bounded status-health probes, neutral unavailable-state
presentation, settings validation/repair and explicit 1,800-second product plus
30-second timer-test profiles. M7 `sdkconfig.m7.defaults` is now the normal
product profile; its separate trace overlay is test-only. Rest-view movement
cancels the entire gesture, while LED output uses 26% selected and 17%
post-effect stability floors when enabled. These floors do not alter the stored
brightness, the display range or commanded shutdown behavior.

M8 retains the M7 product profile. The semantic transport now records exact
first-transmission/correlated-response timing, and the host analyzer combines
that with M7 UI trace markers to enforce the 100 ms and 250 ms limits. The
test-only `sdkconfig.m8.endurance` overlay requests a finite 3,600-second
session through the normal facade; it is not a product default.

The M4 single-character semantic diagnostic console is now guarded by
`CONFIG_SC_ENABLE_SEMANTIC_CONSOLE`. Only `sdkconfig.m4.defaults` opts in;
product, timing and endurance profiles compile it out so host serial bytes
cannot issue application audio commands.

The separate R01 PWM development console is guarded by
`CONFIG_SC_ENABLE_LIGHT_DEBUG_CONSOLE` and is mutually exclusive with the M4
semantic console so only one task reads USB stdin. Commands are `pwm freq
<1..25>`, `pwm duty <0..100>`, `pwm auto`, `pwm status` and `help`. Manual duty
does not persist and normal GUI/effect output resumes on `pwm auto` or reset.

The separate ADR-0011 touch-debug pair uses
`sdkconfig.r01.touch-debug.defaults` on Control and
`sdkconfig.touch-debug.defaults` on Audio. Five rapid rest-view taps enter a
high-contrast grid. Lighting debug mode disables effects, holds exact physical
duty independent of the normal 5–80% mapping, and preserves a separate enable
latch. The Audio image generates C3–C8 directly in code and advertises optional
source/note commands. Exit restores 3 kHz, normal effects, enabled light,
music/C5 and the normal rest view. Duty/frequency/source/note are volatile;
volume remains Audio-owned and persistent.

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
- the bounded R00 quadratic volume law capped at -12 dBFS, with Audio
  Controller NVS persistence and validation;
- normal 30-minute and separately built finite 60-minute endurance configurations;
- 60-second frame/byte/loop/deadline heartbeat logging;
- short-press PLAY/STOP and one-second-hold volume cycling as the M2-only local
  standalone test interface;
- a deterministic, loop-safe validation-WAV generator.

M8 additionally validates that the WAV data chunk declared by the header fits
inside the actual file before playback becomes available. A truncated file now
fails startup validation with an actionable diagnostic instead of deferring the
read failure until playback.

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

Verified for M10 R01 software:

- COB mapping policy tests pass under strict C11 warnings and UBSan for 0%, 1%,
  midpoint, 100%, lower/upper saturation, sub-floor fade values, non-finite
  input and invalid floor/cap fail-off;
- retained frame, semantic-protocol and settings-policy suites pass unchanged;
- all three timing-analyzer unit tests pass;
- a clean ESP-IDF v6.0 ESP32-C6/8 MB R01 Control build passes with GPIO8,
  3 kHz, 5–80% duty and embedded version `R01-dev`; the 586,640-byte image has
  SHA-256 `db5783c01517e3fffe2718f65ca9f39fb0498a5c13b57f985a302ebb2f570345`;
- source/configuration inspection confirms COB LEDC timer 1/channel 1 is
  distinct from display timer 0/channel 0 and Control RGB/RMT code is removed.
- strict C11/warnings-as-errors/UBSan PWM-console parser tests pass for 1–25 kHz,
  decimal 0–100% duty, auto/status/help and rejected invalid inputs;
- the final clean ESP-IDF v6.0 R01 PWM-console build passes at 596,336 bytes,
  SHA-256
  `d14b5ea978dd5b5b20e4c706c24cb4f64a96d2d6219ae0220ec713e799ca2043`;
- that exact image is flashed to the Waveshare and every esptool write passed
  hash verification.

Verified on the USB-connected Waveshare and Atom on 2026-10-25:

- `/dev/ttyACM0` was reconfirmed as Waveshare identity `AC:EB:E6:1E:67:C4`;
- reset boot reported `R01-dev`, GPIO8 default requested 3,000 Hz / actual
  3,001 Hz at 13-bit, and normal display initialization;
- Audio UART synchronized to the Atom and remained `STOPPED`, volume 59%, with
  no fault throughout the PWM command sweep;
- USB Serial/JTAG line handling, `pwm status`, fixed `pwm duty 42.12`, frequency
  selection and `pwm auto` all operated correctly;
- driver-reported applied results were 1,000 Hz/13-bit, 8,993 Hz/13-bit,
  10,000 Hz/12-bit, 19,011 Hz/12-bit, 20,000 Hz/11-bit and 25,000 Hz/11-bit;
- the board was restored to requested 3 kHz / actual 3,001 Hz and automatic
  GUI/effect control before monitor exit.

Verified on the USB-powered Waveshare on 2026-10-23 with 12 V disconnected:

- stable identity `AC:EB:E6:1E:67:C4` resolved to `/dev/ttyACM0`;
- exact 586,640-byte R01 application plus bootloader/partition table flashed and
  every write passed esptool hash verification;
- reset boot reported `R01-dev`, ESP32-C6 revision v0.2 and 8 MB flash;
- logs reported COB PWM initialization on GPIO8 at configured 3 kHz/13-bit,
  5–80% limits and 50 Hz light-task update;
- display initialization completed and UART synchronized with the Atom on
  `/dev/ttyACM1`, reporting authoritative `STOPPED`, volume 59%, no fault.

Verified at the GPIO8 gate with a 20 MHz/20 MSa/s PC oscilloscope on 2026-10-24:

- waveform was visually clean and stable with sharp edges, 0 V low and 3.3 V high;
- carrier measured 3.012 kHz / 332 µs and the period was stable;
- maximum setting varied slowly from approximately 73.5% to 80% duty over
  irregular 20–40 s cycles, consistent with the configured ambient modulation
  and physical duty cap;
- at the minimum active setting, the lowest observed duty was just above 5%,
  confirming the configured active-output floor (the earlier 0.5% report was a
  transcription error);
- at the midpoint brightness setting, ambient variation was approximately
  ±7.5 duty-cycle percentage points;
- GPIO8 sat at 0 V on reset;
- brightness adjustment changed duty responsively, and PLAY/STOP touch actions
  produced the expected smooth positive/negative duty pulse.

Powered prototype evidence reported on 2026-10-24:

- external 12 V and converted 5 V rails were stable;
- both controllers functioned, Audio worked and the COB strip illuminated;
- brightness adjustment was smooth.

The exact external source/PD-trigger and computer-USB state were not reported,
so this does not yet close the final USB-disconnected power topology gate.

Not yet verified for R01: independent scope measurements across the 1–25 kHz
runtime range and fixed-duty accuracy, no-flash behavior with 12 V, MOSFET
temperature, flicker/noise, complete unchanged integrated GUI/settings/audio behavior,
Audio-disconnected operation, and final PD/DC-DC power-cycle behavior. ADR-0010
confirms that a settled 0% target selection through the normal GUI is not
applicable.

Historical R00/prototype evidence follows and does not substitute for R01 COB
target verification.

Verified from the previous Waveshare prototype development:

- Waveshare display/touch stack is operational;
- current UI interaction model is usable and should be preserved unless requirements change;
- the R00 RGB lighting engine was operational before the R01 backend migration;
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

Verified for M5 software and target hardware on 2026-09-23:

- remote-facade implementation `1f6e326` and STOP-presentation correction
  `31d8221` build for the ESP32-C6/8 MB product configuration; inherited strict
  framing/semantic tests pass and the retained local backend still compiles;
- final Control image `31d8221` is 603,152 bytes with SHA-256
  `0ca8ebb10880ab66a9926870da082460826ae6cee5e3098da2e68fe676b7e324`;
  flash writes were hash-verified and the settings partition was preserved;
- rest-view taps issue finite PLAY/STOP through the facade. PLAY feedback follows
  the accepted STARTING state; STOP feedback follows authoritative STOPPING at
  fade onset, while the command remains pending until STOPPED;
- adjustment-view volume changes are serialized, authoritatively confirmed and
  audibly artifact-free; display and GPIO8 LED-strip brightness follows the
  right-hand slider smoothly and in sync;
- placing Audio in download mode during playback stopped output safely. An
  unavailable rest-view tap produced no false PLAY feedback while UI and
  brightness remained responsive;
- Audio restarted silent and did not resume playback. Control negotiated new
  session `508b7be2`, reconciled STOPPED/51%/no fault through GET_STATUS without
  replay, and the subsequent PLAY/STOP regression was clean and synchronized;
- detailed evidence is in
  `docs/software/verification/2026-09-23_M5_control_audio_facade.md`.

Verified for M6 software and target hardware on 2026-09-23:

- one-second GET_STATUS probes detect a quiet broken return path after three
  bounded 250 ms attempts, select neutral/unavailable audio presentation, and
  leave display, lighting and brightness control responsive;
- the 30-second accelerated finite-session path reached TIMER_EXPIRED, performed
  its five-second fade and became silent/STOPPED in approximately 30.1 seconds
  total. The visual stop transition began with the fade and selected brightness
  returned unchanged after the intentional transient dip;
- with Atom GPIO1 TX → Waveshare GPIO17 RX physically disconnected during
  playback, Audio independently reached the original deadline and stopped.
  Reconnection used HELLO/GET_STATUS to reconcile TIMER_EXPIRED/STOPPED without
  replaying PLAY;
- resetting only Control during a separate active session did not restart,
  change level, cancel or extend Audio playback. The new Control session
  reconciled after Audio stopped at the existing deadline;
- a normal dual-controller power cycle restored Control brightness 42% and
  Audio volume 63%, mounted/validated content, and remained silent/STOPPED with
  no automatic playback;
- final Control `9e28ab7` is 603,680 bytes, SHA-256
  `a601dab0f8c9308e790419bf83c3afa97c361b9d2a4456ff5c65954c785adda1`;
  its hash-verified flash and boot log show ESP32-C6/8 MB and
  `product UI=1800s`. Deployed Audio `d373b1a` is 341,680 bytes, SHA-256
  `917469ceab5145273ebaac7577bc70d3bedaf01cd2c02c91483ded2f22a18411`;
- detailed evidence and limitations are in
  `docs/software/verification/2026-09-23_M6_session_fault_integration.md`.

Verified for M7 software and target hardware on 2026-09-23:

- `727285d` suppresses short-tap release after significant rest-view movement;
  two moved gestures logged `gesture_cancel`/`click_suppressed` and caused no
  playback, slider entry or response pulse;
- a stationary long press measured 1,020 ms, entered adjustment view smoothly,
  and returned after approximately three seconds with current values preserved;
- short-tap PLAY/STOP responses were synchronized with audio start/fade, direct
  brightness changes kept display/strip coordinated, and authoritative Audio
  presentation remained correct;
- initial dark-room testing found a clean full-range display but colorful strip
  quantization below the user-selected 26% clean level and briefly during the
  STOP dip. `f776eb6` applies LED-only 26% selected and 17% post-effect floors;
  corrected testing stayed warm white at minimum and through STOP. Slightly
  coarser animation at this low physical output is accepted;
- disconnecting the Audio return path kept rest/brightness responsive, retained
  confirmed volume 63% while ignoring volume input, and produced neither sound
  nor false playback. Reconnection reacquired STOPPED/no-fault without Control
  reset or PLAY replay;
- strict host regressions pass. Exact `65ffc61` Control product and trace builds
  are 604,032 bytes / SHA-256
  `52ff3c8015882a2612762835af5071f16d19790b2c18d44d0832e02d43f4bc25`
  and 604,704 bytes / SHA-256
  `2d438044bccf83e80936f6ff211cfb80e80c4cafc76204e7342f2d5755e243ef`;
- the product image was restored with trace disabled and reconciled
  silent/STOPPED. The user confirmed normal display and LED operation. Full
  evidence is in `docs/software/verification/2026-09-23_M7_ux_lighting.md`.

Additional non-blocking permutations not run on target:

- exhaustive SRS-AUD-009 corrupt-content permutations beyond the passing M2
  absent-card/unsupported-format and M8 missing-file/truncated-data cases;
- physical injection of an invalid Control brightness NVS value. Policy unit
  tests cover the bounds, and retained M2 target evidence covers invalid Audio
  volume, so this is not an open R00 requirement gap.

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
- ADR-0009: R01 fixed-color COB PWM lighting and distinct development/final
  power arrangements; supersedes only RGB-specific ADR-0002/ADR-0003 clauses.

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
above 4 kHz or by a 12 dB digital-gain comparison. A Speaker Base headphone-jack
comparison was invalid because the jack directly shares the filterless
differential Class-D output; severe high-pitched noise occurred and that test
should not be repeated with those headphones. The exact mono asset was later
auditioned through the laptop headphone output: similar but less noticeable
background noise appeared only at high volume, and unrelated Spotify material
showed comparable noise. Intended-distance playback is accepted for R00 without
claiming the physical cause resolved; future mitigation remains a post-R00
improvement. The available 1 GB FAT16 card has
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

The following post-R00 improvement remains intentionally open:

- physical root cause and potential mitigation of the accepted low-level
  `AUD-ISS-001` playback noise. Its R00 release disposition is complete.

The R00 volume curve and transition timings are no longer open placeholders.
Any later perceptual retuning is an intentional behavior change that requires
repeating the affected target evidence.

R01 target lighting behavior is the current release blocker. GPIO8 is an
ESP32-C6 strapping pin; an external gate pull-down is mandatory, and reset/boot
must be scoped before 12 V is applied. Carrier accuracy, duty points, unintended
flash, visible flicker, MOSFET heating and PWM-related noise are unverified.
Final common-supply operation is also unverified. During development, do not
connect the external DC-DC 5 V output to either controller, and establish an
explicit external-source/MOSFET-source connection to Waveshare ground before
applying 12 V.

Observed M3 setup issue: the Waveshare USB Serial/JTAG endpoint failed to
enumerate after manual BOOT/RESET (kernel error `-71`), then returned after a
plain RESET. The ESP-IDF automatic-reset flash succeeded. This did not indicate
an M3 UART or firmware failure, but boot/download-mode handling should be
watched in future flashes. M4 confirmed that the Waveshare can enter download
mode automatically for flashing; no manual BOOT/RESET preparation is required.
Do not hot-unplug one controller while its UART lines are connected to the
powered other board.

## 6. Next recommended task

**Verify the touch-debug controls and generated/PWM outputs.**

The exact next task is:

1. With external 12 V initially off, confirm five-tap entry, the eight controls,
   GPIO8 low/reset safety, exact-duty/light-latch independence and 1–25 kHz
   selection.
2. Apply the already proven prototype power topology and measure selected PWM
   frequencies/duties while checking flicker, MOSFET temperature and noise.
3. Verify C3–C8 generated tones, music/tone fades, unchanged finite deadline,
   effect suppression and B4 restoration to normal UI/music/C5/3 kHz/effects.

## 7. Session closeout record

### 2026-10-31 — Matched touch-debug pair flashed and boot-verified

- Rebuilt both development profiles cleanly with ESP-IDF v6.0 after the prior
  temporary build directories had been cleared. Artifact sizes remain 593,680
  bytes for Control and 343,488 bytes for Audio.
- Identified Atom ESP32-S3 MAC `44:1b:f6:f4:fe:c8` on `/dev/ttyACM0` and
  Waveshare ESP32-C6 base MAC `ac:eb:e6:1e:67:c4` on `/dev/ttyACM1`; the ACM
  order was the reverse of the previous development session.
- Flashed both exact images. Bootloader, partition-table and application writes
  all passed esptool hash verification. Application SHA-256 values are
  `0f989121ff1719e07e1490ff10c3248d0a45d4f06d6d6694dea9699f7daf21df`
  (Control) and
  `101ce82c3fb37ec373e2a9f6af15bc3aa156bc4c34e022c7927090a4f0fef36a`
  (Audio).
- Control booted `R01-touch-debug`, initialized GPIO8 at requested 3000 Hz /
  actual 3001 Hz, synchronized to Audio with capability mask `0x000f`, and
  reconciled to `STOPPED`. Audio booted `R01-touch-debug-audio`, mounted the SD
  card, validated the canonical WAV and reached silent `STOPPED` at persisted
  50% volume.
- No five-tap/grid interaction, generated-tone listening, powered duty/frequency
  measurement or exit-restoration behavior is claimed yet. The exact next task
  is the three-step interaction gate in section 6.

### 2026-10-31 — Matched touch-debug image implemented and clean-built

- Created `feat/r01-touch-debug-ui` as a separate development-image branch.
- Added five-rapid-tap recognition and a high-contrast 2-by-4 grid for
  PLAY/STOP, volume, light enable, exact duty, source, PWM frequency, C-note and
  exit. Normal profiles compile the grid out.
- Exact B2 control is physical 0–100% PWM duty and bypasses the 5–80% product
  mapping. It is volatile, independent of the B1 enable latch, and clears on
  exit. Debug mode disables COB effects and fixes display backlight at 70%.
- Added optional capability-negotiated UART source/note commands plus a local
  44.1 kHz C3–C8 sine generator. Active changes use one-second down/up fades
  without replacing the finite deadline. Exit restores music/C5, including
  deferred restore after Audio reconnect.
- Added ADR-0011, ICD/UX/SAS/IMP/VVM updates and software evidence report
  `verification/2026-10-31_M10_touch_debug_image.md`.
- Strict C11/UBSan semantic protocol and generated-tone tests pass. Clean
  ESP-IDF v6.0 matched builds pass at 593,680 bytes for Control and 343,488
  bytes for Audio with the intended identities.
- Clean normal-profile regression builds also pass: Control is 597,680 bytes
  with `CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE` unset and Audio is 342,160 bytes
  with `CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO` unset.
- At this implementation checkpoint no image had been flashed. The later
  closeout entry above records the matched flash and boot smoke; interactive
  touch/PWM/audio behavior remains unverified.

### 2026-10-25 — Terminal compatibility and quiet-console update verified

- User confirmed commands work after configuring the serial terminal for CR+LF
  and reported that periodic `audio_link` status messages clutter the monitor.
- Updated the bounded console accumulator to accept CR, LF or CR+LF without
  double-submitting CR+LF commands.
- While the R01 PWM console is active, `audio_link` info logs are now suppressed;
  warnings and errors remain visible so transport faults are not hidden.
- Strict parser tests pass. The ESP-IDF v6.0 image builds at 596,992 bytes with
  SHA-256
  `6101e5ae04b8585a686b0045f400834e9eb4253e600d2551ebfa1f9ac31818b3`.
- The first flash attempt could not acquire `/dev/ttyACM0` while the user's
  terminal held it and stopped before any write. After the terminal was closed,
  the exact image was flashed to the confirmed Waveshare on `/dev/ttyACM0`;
  bootloader, partition table and application all passed write/hash checks.
- Boot confirmed `R01-dev`, requested 3000 Hz/actual 3001 Hz at 13-bit, and the
  explicit quiet-console startup message. Target commands terminated by CR,
  LF and CR+LF each produced exactly one status response. No repetitive
  `audio_link` info messages appeared during the checks; warnings remain
  available by design.

### 2026-10-25 — PWM debug image flashed; serial runtime sweep passed

- Reconfirmed the Waveshare as `/dev/ttyACM0`, identity
  `AC:EB:E6:1E:67:C4`; the Atom remained `/dev/ttyACM1`.
- Initial target interaction exposed USB Serial/JTAG nonblocking partial-line
  delivery. Replaced `fgets` handling with a bounded byte accumulator supporting
  CR/LF, backspace and overlength-line rejection; complete commands then passed.
- The first 25 kHz request exposed the ESP32-C6 shared LEDC clock constraint.
  The safe rollback retained 3 kHz and fixed duty. Both the display and COB
  timers are now pinned to the shared 80 MHz PLL-divided source, while retaining
  timer 0/channel 0 and timer 1/channel 1 respectively.
- Strict parser and PWM-policy tests pass. A fresh clean ESP-IDF v6.0 build is
  596,336 bytes with SHA-256
  `d14b5ea978dd5b5b20e4c706c24cb4f64a96d2d6219ae0220ec713e799ca2043`.
  That exact image was flashed and all writes passed esptool hash verification.
- Reset boot, display initialization and Audio UART reconciliation passed. With
  fixed requested duty 42.12%, serial runtime checks passed at 1, 9, 10, 19, 20
  and 25 kHz with expected 13/12/11-bit transitions. The exact applied values
  are recorded in section 3 and the M10 verification report.
- Restored requested 3 kHz / actual 3,001 Hz and automatic GUI/effect control.
  Independent scope measurements and powered thermal/flicker/noise checks are
  the exact next task in section 6.

### 2026-10-24 — Powered prototype partial pass; PWM debug console implemented

- User reports stable external 12 V and converted 5 V rails, functioning Control
  and Audio controllers, working Audio/COB output and smooth brightness control.
  Exact PD-trigger/USB state plus reset, thermal, flicker and noise observations
  remain unverified.
- Added an R01 development-only USB console with `pwm freq <1..25>`, `pwm duty
  <0..100>`, `pwm auto`, `pwm status` and `help` commands. Frequency is selected
  in 1 kHz steps. Duty accepts hundredths of a percent and is quantized by LEDC.
- Manual duty bypasses GUI, ambient, PLAY/STOP and normal 5–80% limits without
  changing persisted brightness. Releasing the override resumes current normal
  state. Frequency/duty overrides are volatile and reset to 3 kHz/automatic.
- Runtime frequency changes are owned by the lighting task. LEDC resolution is
  13 bits at 1–9 kHz, 12 bits at 10–19 kHz and 11 bits at 20–25 kHz.
- Strict parser, retained COB-policy/frame/semantic/settings host tests and all
  three timing-analyzer tests pass. A clean ESP-IDF v6.0 build passes at 596,176
  bytes with SHA-256
  `6e0f32835aebb2ff06c177d0841f4a3c88d5535c084198d61d0385dfae6519dc`.
- The new image is not flashed or runtime-tested. The exact next task is the
  safe flash and frequency/duty scope sweep in section 6.

### 2026-10-24 — GPIO8 PWM scope gate passed; GUI minimum clarified

- User measured GPIO8 with a 20 MHz analog-bandwidth / 20 MSa/s PC oscilloscope.
  The waveform was clean and stable with sharp 0 V/3.3 V transitions.
- Carrier measured 3.012 kHz (332 µs), within approximately +0.4% of the 3 kHz
  target, with a stable period.
- Maximum brightness slowly varied from approximately 73.5% to the 80% cap over
  variable 20–40 s cycles. The minimum active output remained just above the 5%
  floor; an earlier 0.5% value was corrected as a transcription error. At the
  midpoint brightness setting, ambient excursion was approximately ±7.5 duty
  points. Adjustment response and the smooth PLAY/STOP kick were observed.
- GPIO8 was observed at 0 V on reset.
- These results verify carrier/logic levels and demonstrate ambient/audio duty
  effects, the active 5% floor, midpoint modulation and reset-low behavior.
- The user confirmed the latest intended GUI decision: normal brightness
  adjustment retains the 5% active minimum and does not expose an off setting.
  ADR-0010 records the distinction between GUI minimum and internal/reset/fail-
  safe 0% duty. A GUI-selected settled-off capture is therefore not applicable.
- M10-HWT-001 passes. No powered 12 V COB observation was reported; the exact
  next task is the powered development-topology check in section 6.

### 2026-10-23 — Exact R01 Control flash and boot initialization pass

- Confirmed both modules USB-connected with 12 V disconnected. Stable identities
  are Waveshare `/dev/ttyACM0` (`AC:EB:E6:1E:67:C4`) and Atom `/dev/ttyACM1`
  (`44:1B:F6:F4:FE:C8`).
- Reverified staged application SHA-256
  `db5783c01517e3fffe2718f65ca9f39fb0498a5c13b57f985a302ebb2f570345`,
  then flashed it to the Waveshare. Bootloader, partition table and application
  writes all passed hash verification.
- Captured a deliberate reset boot. The target reports `R01-dev`, GPIO8 COB PWM
  configured for 3 kHz/13-bit and 5–80% duty limits, successful display startup,
  and successful UART synchronization to Atom `STOPPED`/59%/no fault.
- No scope/logic-analyzer measurement or 12 V test was performed. M10-HWT-001 is
  now PARTIAL; M10-IT-001 and M10-PWR-001 remain pending.
- Exact next task remains verifying GPIO8 reset-low behavior and measuring off,
  1%, midpoint and 100% frequency/duty values before applying 12 V.

### 2026-10-19 — M10 COB PWM software implementation

- Created `feat/r01-cob-pwm-lighting` from clean tag `software-r00`; existing
  R00 branches, reports and tag were not changed.
- Replaced Control RGB/RMT lighting with a GPIO8 active-high LEDC PWM driver and
  host-testable logical-brightness mapping while retaining the `light_service`
  API, GUI, persistence and whole-strip startup/ambient/PLAY/STOP effects.
- Added `sdkconfig.r01.defaults`, application version `R01-dev`, ADR-0009 and
  R01 updates to the active SRS/UX/SAS/ICD/VVM/plan/README.
- Implementation checkpoint `a3036fd` contains the complete firmware/document
  change; the exact clean build recorded below was produced from that checkpoint.
- New mapping tests and retained settings/frame/semantic/timing tests pass. A
  clean ESP-IDF v6.0 R01 Control build passes. See
  `verification/2026-10-19_M10_cob_pwm_implementation.md` for exact artifact
  identity and commands.
- No target flash, frequency/duty measurement or powered-strip test was
  performed. M10-HWT-001, M10-IT-001 and M10-PWR-001 remain pending.
- Exact next task is the five-step unpowered-strip GPIO8 measurement gate in
  section 6; do not apply 12 V or create an R01 tag before that gate passes.

### 2026-09-27 — M9 passes and software R00 is released

- Resolved the stable USB identities as Control `AC:EB:E6:1E:67:C4` on
  `/dev/ttyACM0` and Audio `44:1B:F6:F4:FE:C8` on `/dev/ttyACM1`.
- Flashed the exact M9 Control and Audio artifacts; every bootloader, partition
  table and application write passed esptool hash verification.
- Non-resetting monitors showed repeated synchronized `STOPPED`, persisted
  volume 59%, `remaining=0`, `fault=0`. The user reported normal startup,
  speaker silence, display and LED-strip operation.
- Product input issued `PLAY(3600s)`; Audio established the 3,600-second local
  deadline and both sides reported `PLAYING` with a matching countdown and
  `fault=0`.
- Manual STOP entered `STOPPING`, completed the one-second fade, and remained
  silent `STOPPED`, reason 1, `remaining=0`, `fault=0`, without restart. The
  user reported all audio/visual behavior normal.
- Promoted the active document set from draft to R00, closed the M9 VVM/plan
  gates, and designated annotated tag `software-r00` for the final release
  commit. `AUD-ISS-001` remains an accepted R00 limitation and post-R00 work.

### 2026-09-27 — M9 release preparation reaches target-smoke gate

- Audited the active specification and ADR set. All R00-critical behavior is
  resolved, ADR-0002 through ADR-0008 remain accepted, and `AUD-ISS-001`
  retains its explicit accepted-limitation disposition.
- Removed the obsolete Waveshare-local audio sources, Kconfig surface, Helix
  dependency and SPIFFS image generation from the active Control tree. The
  unchanged partition layout retains an unused storage partition to avoid an
  unnecessary release migration.
- Added `sdkconfig.r00.defaults` for the ESP32-C6/8 MB product image with a
  finite 3,600-second request, validated UI/light configuration, and diagnostic
  console/trace disabled. Both projects now embed stable version `R00`.
- Strict C11/UBSan frame, semantic-protocol and settings-policy suites pass;
  all three timing-analyzer tests pass.
- Fresh exact ESP-IDF v6.0 builds from `2bb7fd0` pass: Control 603,200 bytes,
  SHA-256 `63f1b0bd27ca47ca9583d4c90ca6a13da16dfe5577c937af4a0c4bc5d5836ef3`;
  Audio 341,952 bytes, SHA-256
  `bf088239b393820beb72b29635f940d6c03de9d9a80ef5142271354a5b28c2af`.
- A first parallel Control build ran out of `/tmp` space at final link. This was
  a host-capacity failure, not firmware evidence; removing generated obsolete
  build trees and repeating the same clean command passed.
- No `/dev/ttyACM*` device is currently enumerated. M9 is paused only for both
  controllers to be connected so the exact-image integrated smoke can run.

### 2026-09-27 — R00 60-minute product-default regression passes

- Resolved both USB identities before flashing: Waveshare Control
  `AC:EB:E6:1E:67:C4` on `/dev/ttyACM0` and Atom Audio
  `44:1B:F6:F4:FE:C8` on `/dev/ttyACM1`.
- Detected that an initial Control `idf.py flash` reconfigured the preserved
  build against documentation-only checkpoint `8fdcb2b`. No target evidence
  was credited to that interim image. Rebuilt from detached implementation
  commit `3348213` and directly flashed the corrected exact candidate; Audio's
  preserved exact `3348213` candidate was also directly flashed. All writes
  passed esptool hash verification.
- Final tested candidate identities are Control 603,216 bytes, SHA-256
  `c526231c244d70e2036069c90cd709019b1472d947b4c5e5918dcb20958e0036`,
  and Audio 341,952 bytes, SHA-256
  `d62b3d311f9de8366e794b4212019447f3c16834ac6328db31096d75d6e0dd74`.
- Silent synchronized startup passed at persisted volume 59%: repeated status
  reported `STOPPED`, `remaining=0`, `fault=0`; the user confirmed normal rest
  display/strip behavior and no click, pop or false playback indication.
- Control requested `PLAY(3600s)`; Audio logged `session start duration=3600s`.
  Both sides reported `PLAYING` from 3,600 seconds with a normal countdown and
  `fault=0`.
- Manual STOP passed: Audio logged its one-second fade, reached silent
  `STOPPED`, reason 1, `remaining=0`, `fault=0`; repeated polls showed no
  restart. The user confirmed good synchronized response, a clean fade and a
  fully silent speaker.
- `R00-DUR-001` passes using this targeted exact-candidate result plus retained
  M8-END-001 full-hour evidence. M9 release preparation is the exact next task.

### 2026-09-26 — R00 60-minute product-default adjustment in progress

- User approved changing the normal R00 session default from 1,800 to 3,600
  seconds while retaining PCM WAV content and deferring SD configuration/log
  features.
- Updated current Control product defaults, Audio local fallback, SRS, ICD,
  operating instructions and ADR-0008's explicit duration note. The protocol
  remains finite at 1–3,600 seconds and ownership/fault behavior is unchanged.
  Historical M5/M6 defaults and verification records retain 1,800 seconds to
  describe their actual milestone images.
- Strict frame, semantic-protocol and settings-policy C11/UBSan tests pass; all
  three timing-analyzer unit tests pass. Fresh preliminary ESP-IDF v6.0 product
  builds pass at 603,216 bytes (Control) and 341,952 bytes (Audio). Generated
  Control configuration is 3,600 seconds and Audio identifies a 60-minute local
  default.
- Exact clean product builds from implementation `3348213` pass. Control is
  603,216 bytes, SHA-256
  `c21fe2262e588443589f651a9fb2d779f017bd779a363a106cacc365413343d3`;
  Audio is 341,952 bytes, SHA-256
  `d62b3d311f9de8366e794b4212019447f3c16834ac6328db31096d75d6e0dd74`.
  Both embed `3348213`; generated Control configuration is 3,600 seconds and no
  product build enables an endurance overlay.
- M8-END-001 already passes the full 3,600-second distributed runtime path. The
  remaining targeted gate is exact candidate flash, silent synchronized
  startup, one 3,600-second PLAY confirmation and clean manual STOP. The earlier
  `b69d1a9`/`057f1c5` M8 product pair is not the final candidate for this amended
  behavior.

### 2026-09-26 — M8 endurance, final product restore and completion

- Normal restored-content boot passed before endurance: Audio `057f1c5`
  validated the 6,009,855-frame canonical WAV, loaded persisted volume 59% and
  reached silent `READY`/`STOPPED` with Control synchronized.
- Flashed and exercised the finite 3,600-second Control endurance profile. The
  exact target artifact reported `b69d1a9`, was 603,216 bytes, and had SHA-256
  `b418070bbf551c71aec6c446969a37b63154368e36f974efb2557443fd8fcafb`.
  This differs from the earlier `ac421a7` artifact only because subsequent
  documentation commits advanced the application descriptor; functional
  Control source remained the corrected `ac421a7` implementation.
- The distributed session began at 59% and Audio heartbeats advanced through
  132,301,312 frames / 529,205,248 I2S bytes, 22 content loops and 600 seconds
  remaining. Control status was captured through 543 seconds remaining, after
  which host PTY backpressure compressed the accumulated interval. Final
  Control polling repeatedly reported `STOPPED`, remaining 0, timer-expired
  reason 2 and `fault=0`. The user observed the full hour and reported no
  artifact.
- One rejected Audio frame occurred during the pre-session Control flash/reboot
  and recovered before PLAY. No endurance-session target fault, panic, watchdog,
  reboot, underrun or partial I2S write was observed. The incomplete serial
  interval is explicitly retained as a host-capture limitation.
- Restored the normal product profile. Its exact `b69d1a9` artifact is 603,216
  bytes with SHA-256
  `0a6b91a7ce86003a4a4d79d3c5b79151927fe06960ad192a3a708f9ba7585306`;
  flash write hashes passed.
- Three coordinated dual-controller power cycles passed. Each Audio log showed
  a fresh session, canonical content, saved 59% volume, silent `STOPPED` and
  resumed Control GET_STATUS traffic. The user confirmed silent startup, no
  automatic playback, normal display/GPIO8-strip operation and stable
  brightness/volume around 55–60% on every cycle.
- M8 passes. All non-deferred SRS/accepted ICD evidence gates are satisfied for
  entry into M9. `AUD-ISS-001` remains an accepted R00 limitation and post-R00
  improvement, not a release blocker. Exact next task: execute the M9 active-
  document/ADR audit and release promotion sequence in section 6.

### 2026-09-26 — M8 content faults and audio-noise disposition

- Explicit wiring inspection passed: TX, RX and common GND are connected; the
  inter-controller red 5 V contact is disconnected.
- Preserved the exact 12,019,788-byte canonical WAV before fault injection:
  signed-16 mono PCM at 44.1 kHz, 136.277891 seconds, SHA-256
  `04295f9b56eec65350f8c3b566f04ec43409aa4b2685b37db52f763dc3a4e963`.
- Missing-file boot passed with `ESP_ERR_NOT_FOUND`, silent `FAULT`, playback
  disabled, continued UART polling and responsive display/GPIO8 strip.
- A separate 4,096-byte copy retaining the canonical header was rejected before
  playback with `declared_end=12019788 file_size=4096` and
  `ESP_ERR_INVALID_SIZE`. Output remained silent, Control UI/lighting remained
  responsive and no panic/reboot occurred.
- Restored `/sleepcube/audio.wav` from the untouched on-card original and
  re-hashed it against the laptop safety copy; format, duration, size and hash
  all match. The fault specimen remains outside the lookup path as
  `/sleepcube/audio.m8-truncated.wav`.
- Auditioned the exact canonical mono file through laptop headphones. Similar
  but less noticeable noise was heard at high volume, and unrelated Spotify
  music through the same path exhibited comparable background noise. Together
  with intended-distance acceptability, three-speaker evidence and clean
  silence/transitions, `AUD-ISS-001` is explicitly accepted as a documented R00
  limitation. Root cause remains a post-R00 improvement, not a release blocker.
- Normal sandbox operation was restored after identifying and unmounting a
  malformed Snap `mesa-2404` namespace entry; this was a host-tooling issue and
  did not affect target behavior or evidence.
- Exact next task: normal restored-content boot, then the integrated 3,600-second
  endurance run and final product/persistence restore. M8 remains incomplete.

### 2026-09-24 — M8 quantitative timing and repeated-reset gate

- Flashed/exercised Audio `057f1c5` and the Control M8 timing profile. The
  initial reset attempt exposed an enabled M4 semantic diagnostic console:
  stray host-monitor input issued unintended 75%, 50% and 100% volume commands
  plus STOP and duplicate-request traffic. Audio never started, but persisted
  volume changed from 63% to 100%; that run is superseded and is not acceptance
  evidence.
- Added `CONFIG_SC_ENABLE_SEMANTIC_CONSOLE` in correction `ac421a7`. It defaults
  off and is enabled only by the M4 bring-up defaults. Product, timing and
  endurance builds compile the console out.
- Reflashed exact corrected trace `ac421a7`, restored Audio volume to 59%, and
  reproduced reset/monitor recovery. No semantic-console command, automatic
  PLAY or unexpected volume change occurred.
- Ten corrected PLAY/STOP cycles supplied 20 timing samples. Touch acceptance
  was 0.296/0.395/0.452/0.453 ms min/median/p95/max; complete user command to
  correlated acknowledgement was 47.748/57.749/59.685/59.710 ms. UART RTT was
  39.030/40.011/44.919/51.870 ms. The tracked analyzer passed both normative
  limits.
- Five independent Control resets and five independent Audio resets passed,
  including one active-playback reset of each. Stopped resets stayed silent;
  Control reacquired state without replay or deadline replacement; Audio reset
  stopped output, booted a new session and remained STOPPED.
- Rebuilt exact Control checkpoint `ac421a7`: product 603,216 bytes, trace
  603,904 bytes and endurance 603,216 bytes. Their SHA-256 identities, plus the
  deployed 341,952-byte Audio `057f1c5` identity, are in the M8 verification
  record. No project-code warning was observed.
- Current target state is silent/STOPPED at volume 59% with the corrected trace
  image deployed. The exact next task is the SD-card content/noise fault gate in
  section 6; M8 remains incomplete.

### 2026-09-23 — M8 audit and validation preparation

- Created `feat/m8-system-validation` from completed M7 and audited every active
  SRS, UX and ICD ID. The VVM now includes all IDs; the detailed disposition and
  exact remaining physical sequence are in
  `docs/software/verification/2026-09-23_M8_system_validation.md`.
- Resolved stale deferred-design text. ADR-0007/ADR-0008 already freeze framing
  and retry behavior; the repeatedly validated 1 s start/manual-stop, 250 ms
  volume and 5 s timeout transitions are now explicit R00 criteria.
- Added exact Control transport request/response timestamps and a standard-
  library analyzer for touch acceptance, complete user-command acknowledgement
  and UART RTT. Its three unit tests pass.
- Added the test-only 3,600-second distributed endurance overlay and Control
  product/trace/endurance build, flash, monitor and analysis instructions.
- Hardened Audio startup validation so a declared WAV data chunk must fit the
  actual file before playback is available; truncated content is now rejected
  with an actionable error.
- Strict C11/UBSan frame, semantic-protocol and settings-policy regressions pass.
  Clean ESP32-C6/8 MB product, trace and endurance builds plus the ESP32-S3/8 MB
  Audio build pass without observed project-code warnings. All four were
  rebuilt at exact checkpoint `e4d31cc`; sizes and SHA-256 identities are in the
  M8 verification record.
- Neither controller was enumerated during preparation. No target timing,
  reset, content-fault, endurance, persistence or audio-noise result is claimed.
  The first hardware gate in section 6 is ready to begin when both controllers
  are connected.

### 2026-09-23 — M7 hardware completion

- Flashed and exercised the M7 trace image on the Waveshare while Audio
  `d373b1a`, the display, GPIO8 strip, storage and speaker remained in the
  established product setup.
- Short PLAY/STOP taps, moved-gesture suppression, 1,020 ms stationary long
  press, approximately three-second adjustment timeout, preserved controls and
  synchronized visual/audio responses passed. Trace confirmed that cancelled
  movement emitted no audio-toggle event.
- The initial dark-room run exposed colorful low-count strip behavior below the
  empirically clean 26% level and during the low STOP dip. Correction `f776eb6`
  added LED-only 26% selected and 17% post-effect floors without restricting the
  display slider or stored setting.
- Rebuilt exact application `65ffc61`: product is 604,032 bytes, SHA-256
  `52ff3c8015882a2612762835af5071f16d19790b2c18d44d0832e02d43f4bc25`;
  trace is 604,704 bytes, SHA-256
  `2d438044bccf83e80936f6ff211cfb80e80c4cafc76204e7342f2d5755e243ef`.
- Corrected dark-room retest passed: ambient and STOP response remained stable
  warm white. Slightly coarser modulation at the minimum was accepted as an LED
  resolution characteristic.
- Audio return-path disconnection kept brightness/rest usable and silent,
  retained confirmed volume 63% and ignored volume changes. Reconnection
  recovered authoritative STOPPED/no-fault without reset or replay.
- Restored the exact product image with trace disabled. The user confirmed
  silent output and normal display/LED operation. M7 is PASS; detailed evidence
  is in `docs/software/verification/2026-09-23_M7_ux_lighting.md`. The exact
  next work is the M8 evidence audit in section 6; `AUD-ISS-001` remains open.

### 2026-09-23 — M7 implementation and build preparation

- Created `feat/m7-ux-lighting-refinement` from the completed M6 baseline and
  audited every active UX row against the retained Control implementation and
  M5/M6 distributed-state evidence.
- Fixed the concrete remaining gesture gap: movement beyond the existing
  threshold now cancels the complete rest-view gesture, so its release cannot
  emit an audio-toggle click. Added focused trace points for physical
  touch-to-event timing, gesture cancellation, long press and UI mode changes.
- Added tracked `sdkconfig.m7.defaults` for the normal 1,800-second distributed
  product and `sdkconfig.m7.trace` as a test-only timing overlay. The normal
  product profile keeps high-volume tracing disabled.
- Strict C11/UBSan frame, semantic-protocol and settings-policy tests pass.
  Exact commit `727285d` clean-builds both ESP32-C6/8 MB profiles: product is
  603,744 bytes, SHA-256
  `6091ba840f2ff5c1034689944b912bbca714f49d8395cacb42dd0bef8642348d`;
  trace is 604,432 bytes, SHA-256
  `98532ed85f2fd223b56e98667a9bcf5ebd2d01ac9aed7f85e10c3b4f3ecc94d3`.
- Neither controller was enumerated at this checkpoint. No M7 target test or
  flash was claimed at the initial build checkpoint. Both later enumerated with
  their established identities and the trace image was hash-verified on flash.
- The user confirmed synchronized PLAY and STOP response, clean stop fade, two
  moved gestures suppressed without playback or slider entry, smooth adjustment
  transitions, about-three-second return, preserved values, coordinated
  brightness adjustment and a usable low-brightness rest view. Trace measured
  long press at 1,020 ms and captured `gesture_cancel`/`click_suppressed`
  without a toggle event.
- Initial dark-room observation found the display clean through its full range,
  but the strip developed a colorful low-count quantization effect below the
  user-selected 26% acceptable level; the normal STOP dip briefly re-entered
  that region. The user reported warm color, smooth modulation and good sync at
  and above 26%.
- Correction `f776eb6` adds configurable LED-only guards: 26% minimum selected
  output and 17% minimum post-effect output after startup. The display retains
  its complete range and the normal ambient modulation remains. Both product
  and trace profiles clean-build. At this preparation checkpoint the corrected
  dark-room retest was pending; it was subsequently completed and recorded in
  the M7 hardware-completion entry above.

### 2026-09-23 — M6 hardware completion

- Flashed Audio `d373b1a` and the 30-second M6 Control timer profile with
  hash-verified writes; target logs showed both 8 MB devices, valid content and
  silent STOPPED startup.
- Verified product-UI timer expiry through STOPPING/TIMER_EXPIRED and a smooth
  five-second fade to silent STOPPED. The user confirmed synchronized stop
  feedback and unchanged selected brightness after the normal transient effect.
- Removed only Atom TX → Waveshare RX during active playback. Control detected
  loss through bounded status retries while brightness remained responsive;
  Audio stopped independently. Reconnection reconciled authoritative STOPPED
  state without replay.
- Reset only Control during another active session. Audio retained its original
  deadline and level; restarted Control reconciled without restarting or
  extending playback.
- Stored brightness 42% and volume 63%, restored the normal 1,800-second
  product profile, and power-cycled both controllers. Both settings returned,
  playback remained stopped, and the user confirmed silent startup and normal
  display/LED operation.
- Corrected the diagnostic duration label in `9e28ab7`, rebuilt both Control
  profiles, and flashed the final 603,680-byte product image (SHA-256
  `a601dab0f8c9308e790419bf83c3afa97c361b9d2a4456ff5c65954c785adda1`).
  Final boot explicitly reports diagnostic PLAY 30 s versus product UI 1,800 s,
  restores brightness 42%, and reconciles STOPPED/volume 63%/no fault.
- Updated the M6 report, VVM, implementation plan and this handoff. M6 passes;
  exact next work is the M7 UX/lighting review in section 6. `AUD-ISS-001`
  remains open and is not affected by this result.

### 2026-09-23 — M6 software and build preparation

- Created `feat/m6-session-fault-integration` from the completed M5 baseline.
- Added one-second bounded GET_STATUS health probes so quiet link loss becomes
  unavailable after the existing three-attempt timeout; recovery uses
  HELLO/GET_STATUS and never replays PLAY.
- Finalized the R00 degraded presentation as neutral audio with volume input
  suppressed while normal ambient UI, lighting and brightness continue.
- Added the configurable finite Control PLAY duration, retained 1,800 seconds
  in product defaults and added a clearly marked 30-second timer-test overlay.
- Added tested persisted-setting bounds and repair for Control brightness and
  Audio volume.
- Committed implementation as `709512c` and the reproducible timer overlay as
  `ab214fe`. Strict host tests plus clean M6 Control, Audio and retained legacy
  Control builds pass. Exact image sizes and hashes are in the M6 report.
- Hardware remains unmodified at this checkpoint. Confirmed USB identities are
  Waveshare `/dev/ttyACM0` (`AC:EB:E6:1E:67:C4`) and Atom `/dev/ttyACM1`
  (`44:1B:F6:F4:FE:C8`). Exact next action is the five-step physical gate in
  section 6, beginning with manual Atom download mode.

### 2026-09-23 — M5 implementation and hardware completion

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
  `508b7be2` at STOPPED/51%/no fault without replay. The final recovered
  PLAY/STOP regression was clean and audio/light feedback followed both taps.
- M5 passes both exit criteria. Exact next action is the five-step M6 entry task
  in section 6. `AUD-ISS-001` remains open and outside the M5 claim.

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
