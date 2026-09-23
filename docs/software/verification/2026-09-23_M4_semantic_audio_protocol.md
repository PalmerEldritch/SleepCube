# M4 semantic audio protocol — development record

**Date:** 2026-09-23  
**Status:** Software implementation and host/build verification pass; the
two-controller hardware/reset exit gate is pending.

## Scope

M4 replaces the M3 PING/PONG-only task with the version-1 semantic contract in
`../05_ICD.md`. It does not yet connect the retained Waveshare touch UI to
remote audio; that facade migration is M5. The M4 Control image instead exposes
a development USB-console harness so real UART commands can be tested without
changing product UI semantics.

ADR-0008 freezes per-boot Control/Audio session IDs, monotonically increasing
command IDs, one outstanding request, exact retry identity, Audio-side latest-
command duplicate retention, 250 ms response waits, three total attempts and
HELLO/GET_STATUS recovery without automatic replay.

## Implementation prepared

- Shared pure-C protocol code defines HELLO/INFO, GET_STATUS, finite PLAY,
  STOP, SET_VOLUME, RESULT, READY/STATUS/PLAYING/STOPPED/TIMER_EXPIRED/ERROR,
  bounded payload encoders/decoders and the Audio semantic server.
- The Audio endpoint converts validated requests into non-blocking commands for
  the existing sole playback-owner task. The owner now accepts an explicit
  1–3,600 second duration, explicit stop, and 0–100 percent volume while
  retaining the M2 local validation button.
- Synchronized snapshots expose state, confirmed volume, remaining duration,
  stop reason and fault code without giving protocol code access to storage,
  decoder or I2S internals.
- The Control client performs HELLO then GET_STATUS, correlates one request,
  retries with finite waits, marks state unsynchronized after exhaustion, and
  detects a changed Audio session after reset.
- The Control USB console provides `p` (30-second PLAY), `s` (STOP), `1`–`4`
  (25/50/75/100 percent), `q` (GET_STATUS), and `r` (deliberate replay of the
  last state-changing command ID). It is a development harness, not M5 UI
  integration.
- Audio startup remains silent. The semantic endpoint can report NOT_READY or
  startup FAULT while storage/audio initialization proceeds or fails.

## Automated verification

The existing framing suite and new semantic suite pass using the host compiler
with C11, all warnings as errors, pedantic checking and UndefinedBehaviorSanitizer:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/shared/protocol \
  firmware/shared/protocol/sc_frame.c \
  firmware/shared/protocol/tests/test_sc_frame.c \
  -o /tmp/sleepcube-m4-host-tests/test_sc_frame

cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/shared/protocol \
  firmware/shared/protocol/sc_frame.c \
  firmware/shared/protocol/sc_audio_protocol.c \
  firmware/shared/protocol/tests/test_sc_audio_protocol.c \
  -o /tmp/sleepcube-m4-host-tests/test_sc_audio_protocol
```

Coverage includes maximum framing payload and parser resynchronization plus
semantic handshake/status round trips, PLAY/STOP/volume, zero duration,
out-of-range volume, session mismatch, stale/reused IDs, exact duplicate PLAY
suppression, Control-reset reconciliation of a playing Audio state,
malformed/unsupported input, authoritative events and TIMER_EXPIRED encoding.
Both executables report pass.

ESP-IDF v6.0 commit `662a3be354759d9487bf4b1a629fadb766cb1800`
clean-builds the separate 8 MB targets from external build/config paths:

```sh
cd firmware/audio
idf.py -B /tmp/sleepcube-m4-audio-build \
  -D SDKCONFIG=/tmp/sleepcube-m4-audio-sdkconfig \
  set-target esp32s3 build

cd ../control
idf.py -B /tmp/sleepcube-m4-control-build \
  -D SDKCONFIG=/tmp/sleepcube-m4-control-sdkconfig \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.m4.defaults' \
  set-target esp32c6 build
```

Final clean builds from implementation commit `66f36e5` produced the following
images without a project-code compiler warning:

| Controller | Application version | Image | Bytes | SHA-256 |
|---|---|---|---:|---|
| Audio / ESP32-S3 | `66f36e5` | `SleepCube_audio_controller.bin` | 341,568 | `1e34bce1d611b95bb84648190a973ee6dcde90613f001db1146ad897a3cc4aab` |
| Control / ESP32-C6 | `66f36e5` | `SleepCube_P0.bin` | 600,064 | `766a31a6f3b05890d89b9e2f9289d1b26375481c884d2fd2b1eb24aa745195ca` |

These immutable external build directories are the source for the coordinated
hardware flash; the subsequent documentation-only evidence commit does not
alter either image.

## Pending hardware exit gate

1. Identify both controllers by stable USB serial ID. Put the Atom in download
   mode manually, flash its final M4 image, press reset once to start it, and
   capture silent boot/card/content/READY output. Flash the Control M4 image
   without writing its settings storage.
2. Capture HELLO/INFO/GET_STATUS synchronization in both boot orders. Confirm
   no command is issued automatically and the newly connected GPIO8 LED strip
   and display remain responsive.
3. Issue 30-second PLAY from the Control console. Replay it with `r` and verify
   `DUPLICATE`, one playback start and no deadline extension. Observe PLAYING,
   the five-second timeout fade, TIMER_EXPIRED and stopped silence.
4. Exercise manual STOP, 25/50/75/100 percent SET_VOLUME and GET_STATUS. Confirm
   correlated results, authoritative transition events, smooth audio changes
   and no UI/light stall.
5. Reset Control during a 30-second session. Audio must continue only to the
   original deadline; restarted Control must HELLO/GET_STATUS and must not PLAY.
6. Reset Audio during playback. Output must return silent/stopped; Control must
   detect the new Audio session, abandon any pending command and reconcile to
   stopped without its own reset.
7. Record logs, command/result counts, listening/visual observations and any
   limitation. Do not declare M4 complete or begin M5 until both boot orders and
   independent resets pass.

`AUD-ISS-001` remains open and is not resolved by semantic-protocol success.
