# M5 Control audio-facade migration — development record

**Date:** 2026-09-23
**Status:** PASS — software/build, flash/boot, touch audio, authoritative
feedback, volume, brightness independence, Audio-unavailable containment and
post-restart recovery gates complete.

## Scope

M5 replaces the disabled Waveshare-local audio backend with a non-blocking
Control-side facade over the M4 semantic UART client. It preserves the existing
rest-view tap and adjustment-view interaction model while treating Audio
Controller status as authoritative. It does not change the M4 wire protocol,
Audio storage/decoder/I2S ownership, or the open `AUD-ISS-001` disposition.

## Implementation

- `sdkconfig.m5.defaults` selects ESP32-C6, 8 MB flash, USB Serial/JTAG, UI,
  GPIO8 lighting, and the semantic remote-audio backend while disabling the
  legacy local audio path.
- `audio_service` now has a remote facade which polls the synchronized M4
  snapshot, exposes confirmed availability/playback/volume, issues only finite
  1,800-second PLAY requests, suppresses duplicate playback requests, and
  coalesces rapid volume changes to one outstanding UART request plus the latest
  desired value.
- A failed or unconfirmed request does not automatically replay audio. Loss of
  synchronization clears pending facade intent and returns audio controls to an
  unavailable state until M4 HELLO/GET_STATUS reconciliation succeeds.
- Touch commands enter `app_core`; confirmed state/volume events return through
  `app_core` to the display. The rest-view audio pulse is no longer generated
  merely because PLAY was transmitted.
- Audio/light presentation coordination moved from `audio_service` to
  `app_core`. The light service starts before the audio facade and does not
  depend on UART synchronization.
- The existing M4 USB semantic console remains as a development diagnostic,
  with milestone-neutral log labels.

## Automated and build verification

The unchanged framing and semantic protocol suites pass with C11, warnings as
errors, pedantic checking and UndefinedBehaviorSanitizer:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/shared/protocol \
  firmware/shared/protocol/sc_frame.c \
  firmware/shared/protocol/tests/test_sc_frame.c \
  -o /tmp/sleepcube-m5-host-tests/test_sc_frame

cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/shared/protocol \
  firmware/shared/protocol/sc_frame.c \
  firmware/shared/protocol/sc_audio_protocol.c \
  firmware/shared/protocol/tests/test_sc_audio_protocol.c \
  -o /tmp/sleepcube-m5-host-tests/test_sc_audio_protocol
```

Both executables report pass. The retained legacy local-backend configuration
also clean-builds after the facade refactor.

ESP-IDF v6.0 commit `662a3be354759d9487bf4b1a629fadb766cb1800`
clean-built final timing-correction commit `31d8221` from tracked defaults:

```sh
cd firmware/control
idf.py -B /tmp/sleepcube-m5-final-31d8221-build \
  -D SDKCONFIG=/tmp/sleepcube-m5-final-31d8221-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m5.defaults' \
  set-target esp32c6 build
```

The 603,152-byte `SleepCube_P0.bin` targets ESP32-C6/8 MB, has SHA-256
`0ca8ebb10880ab66a9926870da082460826ae6cee5e3098da2e68fe676b7e324`,
and reports a valid image checksum and validation hash. No project-code compiler
warning was observed.

## Hardware progress

USB identity confirmed `/dev/ttyACM0` as the Waveshare ESP32-C6
(`AC:EB:E6:1E:67:C4`) and `/dev/ttyACM1` as the AtomS3 Lite
(`44:1B:F6:F4:FE:C8`). The exact clean M5 image above was written only to the
Waveshare; bootloader, partition table and application writes passed hash
verification. The settings partition was not written.

The automatic post-flash reset produced a complete Control boot log with:

- ESP32-C6 revision v0.2, 8 MB flash and application version `31d8221`;
- UI and GPIO8/28-pixel light services started independently;
- remote semantic audio facade started with legacy local audio disabled;
- HELLO/INFO/GET_STATUS completed after one HELLO retry;
- authoritative Audio state reconciled to `STOPPED`, 50%, zero remaining and no
  fault; and
- no automatic PLAY, panic, watchdog or reboot loop in the captured interval.

The first `1f6e326` touch check found PLAY audio/visual response well synchronized
to the tap, but STOP visual feedback followed the completed one-second audio
fade. Logs showed STOP accepted in approximately 60 ms, immediate authoritative
`STOPPING`, then `STOPPED` approximately 1.02 seconds later. The facade had
intentionally treated STOPPING as active, so presentation waited for STOPPED.

Commit `31d8221` makes authoritative STOPPING end the presented-playing state
and trigger feedback at fade onset, while retaining the pending STOP command
until authoritative STOPPED. This prevents a rapid second tap from starting a
new session during the fade. The correction clean-builds, flashed with hash
verification, booted and reconciled to STOPPED/50%/no fault. The user then
confirmed that the corrected stop response begins with the audio fade.

The adjustment-view volume gate also passes. The user reported that loudness
followed the selected levels without audible artifacts. Audio-side logs show
each accepted value was bounded, ramped over 250 ms and reported back through
authoritative STATUS; the facade reduced continuous touch traffic to serialized
requests with no UART timeout or queue failure. The final confirmed value was
56% after the exploratory drag.

The normal linked brightness gate passes: the user reports that the right-hand
slider changes the display and GPIO8 LED strip smoothly, in sync, and without a
responsiveness problem.

The Audio-unavailable containment gate passes. Playback was active when the
Atom was placed in download mode. The speaker stopped on Atom reset, a
subsequent rest-view tap produced no false PLAY indication, and the Waveshare
display plus GPIO8 strip brightness controls remained responsive. No automatic
retry or uncontrolled output was observed.

The recovery half also passes. The user restarted the Atom and reported silent
startup with no automatic playback. Control then negotiated the new Audio
session `508b7be2`, issued GET_STATUS, and reconciled to authoritative
`STOPPED`, 51%, zero remaining and no fault before marking audio available.
There was no PLAY replay.

The final recovered PLAY/STOP regression passed. Control logged one finite
PLAY(1800s), accepted in approximately 70 ms and confirmed STARTING, followed
by PLAYING. STOP was accepted in approximately 60 ms, confirmed STOPPING at fade
onset, and reached STOPPED approximately 1.04 seconds later. No retry, timeout,
queue failure, fault, panic or reboot occurred. The user confirmed that audio
started and stopped correctly and that audio/light feedback was synchronized
with both taps.

## Exit result

M5 passes both milestone exit criteria: the retained rest/adjustment UI controls
remote Audio through the facade, and lighting remains responsive with Audio
unavailable. M6 still owns release-level timer-expiry reconciliation, degraded
link policy, broader fault injection and persisted-setting finalization.
`AUD-ISS-001` remains open and is not changed by this milestone.
