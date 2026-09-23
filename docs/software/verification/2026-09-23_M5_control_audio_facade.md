# M5 Control audio-facade migration — development record

**Date:** 2026-09-23
**Status:** IN PROGRESS — software/build/flash/boot synchronization pass; touch,
listening and Audio-unavailable UI/light observations pending.

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
clean-built implementation commit `1f6e326` from tracked defaults:

```sh
cd firmware/control
idf.py -B /tmp/sleepcube-m5-final-1f6e326-build \
  -D SDKCONFIG=/tmp/sleepcube-m5-final-1f6e326-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m5.defaults' \
  set-target esp32c6 build
```

The 603,088-byte `SleepCube_P0.bin` targets ESP32-C6/8 MB, has SHA-256
`ba5faa5d821dbb4a8628374d491db099d188d52495d99bf1b1c00ef1b56aaa84`,
and reports a valid image checksum and validation hash. No project-code compiler
warning was observed.

## Hardware progress

USB identity confirmed `/dev/ttyACM0` as the Waveshare ESP32-C6
(`AC:EB:E6:1E:67:C4`) and `/dev/ttyACM1` as the AtomS3 Lite
(`44:1B:F6:F4:FE:C8`). The exact clean M5 image above was written only to the
Waveshare; bootloader, partition table and application writes passed hash
verification. The settings partition was not written.

The automatic post-flash reset produced a complete Control boot log with:

- ESP32-C6 revision v0.2, 8 MB flash and application version `1f6e326`;
- UI and GPIO8/28-pixel light services started independently;
- remote semantic audio facade started with legacy local audio disabled;
- HELLO/INFO/GET_STATUS completed after one HELLO retry;
- authoritative Audio state reconciled to `STOPPED`, 50%, zero remaining and no
  fault; and
- no automatic PLAY, panic, watchdog or reboot loop in the captured interval.

User observation is still required for silent startup, display/light appearance,
touch PLAY/STOP and volume behavior, clean audio transitions, and brightness
responsiveness while Audio is unavailable. No M5 completion claim is made yet.

## Remaining M5 gate

1. Confirm current startup is silent and display/GPIO8 lighting look normal.
2. Short-tap the rest view: verify one finite remote PLAY, confirmed visual
   response and clean audible start; short-tap again and verify a clean STOP.
3. Long-press into adjustment mode, move volume through clearly separated
   values, and verify remote confirmed loudness follows without artifacts or UI
   stalls. Verify brightness remains responsive.
4. Reset the Atom while Control remains running. Verify the speaker stops safely,
   the display/light tasks remain responsive, an audio request while unavailable
   does not show false playing state, and status reconciles after Audio reboot.
