# M4 semantic audio protocol — development record

**Date:** 2026-09-23  
**Status:** PASS — software, host/build, two-controller command, boot-order,
independent-reset, listening and UI/light coexistence gates complete.

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

The first clean preparation builds from implementation commit `66f36e5`
produced the following images without a project-code compiler warning:

| Controller | Application version | Image | Bytes | SHA-256 |
|---|---|---|---:|---|
| Audio / ESP32-S3 | `66f36e5` | `SleepCube_audio_controller.bin` | 341,568 | `1e34bce1d611b95bb84648190a973ee6dcde90613f001db1146ad897a3cc4aab` |
| Control / ESP32-C6 | `66f36e5` | `SleepCube_P0.bin` | 600,064 | `766a31a6f3b05890d89b9e2f9289d1b26375481c884d2fd2b1eb24aa745195ca` |

The flash action re-ran CMake after the evidence commit, so the actually
deployed Audio and initial Control images correctly embedded the then-current
documentation commit `94c7b4a`. The first Control reset exposed a sticky-EOF
defect in the development USB console. Commit `3908d18` adds
`clearerr(stdin)` before retrying and is the final deployed Control image:

| Deployment | Application version | Bytes | SHA-256 |
|---|---|---:|---|
| Audio / ESP32-S3 | `94c7b4a` | 341,568 | `5cf5a22a2abbbe6605de9a2e8e09732c827f157a6f1880247ba81988b31c3de3` |
| Initial Control / ESP32-C6 | `94c7b4a` | 600,064 | `dc2931d6f3c78790d6d12280103089c0d7aadede929945f9c508e27a047fff9a` |
| Final Control / ESP32-C6 | `3908d18` | 600,144 | `353472dc9b1065abbf5d2ad314eb5b10021b70f9a7069c6c6828c67a2e277697` |

All writes completed with esptool hash verification. The Waveshare entered its
bootloader automatically; only the Atom requires manual download-mode entry
and a manual post-flash reset. The Control settings partition was not written.

## Hardware exit gate

The intended AtomS3 Lite (USB identity `44:1B:F6:F4:FE:C8`) and Waveshare
ESP32-C6 (`AC:EB:E6:1E:67:C4`) passed the following on 2026-09-23:

1. Atom boot mounted the 1 GB card, validated the canonical 44.1 kHz mono WAV,
   initialized I2S silent, and reached `READY`/`STOPPED`. Starting Audio before
   Control then produced HELLO/INFO/GET_STATUS synchronization with no automatic
   PLAY. The user confirmed silent startup, normal display operation and normal
   GPIO8 LED-strip operation.
2. SET_VOLUME to 50% was accepted. A 30-second PLAY was accepted once; replaying
   its exact command ID returned `DUPLICATE` at 26 seconds remaining, proving the
   deadline was not restarted. It entered its five-second timeout fade and
   emitted TIMER_EXPIRED/STOPPED after 1,325,056 frames / 5,300,224 bytes. The
   user reported normal start, smooth fade, silent stop and no click, pop,
   dropout or other artifact.
3. A second PLAY followed by STOP produced the expected one-second
   PLAYING/STOPPING/STOPPED transition. The user reported a clean fade. Commands
   for 25%, 75%, 100% and 50% were each accepted and confirmed by STATUS; a
   final GET_STATUS reported STOPPED, 50%, zero remaining and no fault.
4. Control was reset during a third 30-second session. Audio continued without
   interruption or deadline extension, the restarted Control issued only
   HELLO/GET_STATUS, and the original session ended through its normal timeout
   fade after 1,324,544 frames / 5,298,176 bytes. The user confirmed no audible
   restart or extension. Transient malformed/truncated bytes during reset were
   rejected, then valid traffic resumed.
5. The initial Control build's USB harness stopped accepting commands after
   physical reset because stdio retained EOF. The `3908d18` fix built cleanly,
   flashed with hash verification, and accepted GET_STATUS when the monitor was
   attached only after boot, confirming recovery. This was a development-harness
   defect; semantic UART recovery had already passed.
6. Audio was reset during a fourth remote PLAY while Control remained running.
   Output stopped safely. Audio booted silent with session ID changing from
   `03c03e80` to `48b5281d`; Control detected the change, abandoned prior command
   context, performed HELLO/GET_STATUS, and reconciled to STOPPED/50%/zero/no
   fault without replaying PLAY. This also verifies the Control-first boot order.
   The user confirmed no alarming transient or automatic restart.
7. After all command and reset tests the user confirmed that the display and
   GPIO8 LED strip continued to operate normally. No panic, watchdog, playback
   fault or UART command timeout was observed.

M4 passes. Touch-driven product audio control is intentionally still local/
disabled in this build and is the M5 migration task; touch interactions during
M4 can therefore log `ESP_ERR_NOT_SUPPORTED` from the retained local facade.

`AUD-ISS-001` remains open and is not resolved by semantic-protocol success.
