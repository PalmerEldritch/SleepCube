# M1 Audio Controller Bring-up Verification Record

**Date:** 2026-09-16
**Milestone:** M1 — AtomS3 / Speaker Base hardware bring-up
**Implementation branch:** `feat/m1-audio-bringup`
**Status:** PASS — all M1 build, flash, boot/reset, repetition and listening
gates complete

## 1. Configuration under test

| Item | Value |
| --- | --- |
| Audio Controller | M5Stack AtomS3 Lite, ESP32-S3FN8 |
| Audio output | M5Stack ATOMIC Speaker Base, NS4168, supplied 1 W/8 Ω speaker; TEBM 10 W/4 Ω speaker not connected |
| ESP-IDF | v6.0, commit `662a3be354759d9487bf4b1a629fadb766cb1800` |
| M1 source commit | `838248a7195bea285a058e7686523e50e27f5113` (firmware sources match the tested final image; the flashed pre-commit image reports the prior dirty-tree version string) |
| Target / flash image | `esp32s3` / 8 MB |
| Initial I2S format (tested) | 44.1 kHz, signed 16-bit, MSB-aligned mono, master TX |
| Confirmed M1 I2S format | 44.1 kHz, signed 16-bit Philips-I2S stereo, identical left/right samples, master TX |
| I2S pins | BCLK GPIO5, LRCLK GPIO39, DATA GPIO38, MCLK unused |
| DMA | 6 descriptors × 256 frames, auto-clear enabled |
| Trigger | AtomS3 Lite button, GPIO41 active-low |
| Initial diagnostic (tested) | 440 Hz and 1 kHz generated tones, -18 dBFS peak, 250 ms fades |
| Confirmed diagnostic | Same sequence and fades, reduced to -24 dBFS peak |

This is a bring-up configuration, not the final R00 content-format decision.

## 2. Software/build evidence

| Check | Result | Evidence |
| --- | --- | --- |
| Clean out-of-tree configure/build | PASS | `idf.py -B /tmp/sleepcube-m1-final-build -D SDKCONFIG=/tmp/sleepcube-m1-final-sdkconfig set-target esp32s3 build` completed successfully after all M1 source changes |
| Target identity | PASS | Build configured for `esp32s3`; image header chip ID is ESP32-S3 |
| Flash configuration | PASS | Generated image header reports 8 MB, DIO, 80 MHz |
| Image integrity | PASS | `esptool image-info` reports a valid checksum and validation hash |
| Project compilation warnings | PASS | No warning was emitted for `main.c`, `audio_output.c`, or `diagnostic_tone.c` |
| Final application image size | PASS | 196,672 bytes; 81% of the 1 MB application partition remains free |
| Philips-I2S comparison build | PASS | Clean out-of-tree rebuild; 196,672-byte ESP32-S3/8 MB image with valid checksum and validation hash; no project-source warnings |
| USB boot-log capture update | PASS | Added a two-second silent delay before application identity/configuration logs so the native USB endpoint can re-enumerate; rebuilt 196,672-byte image has valid checksum/hash and passed flash/regression testing |
| Repository whitespace check | PASS | `git diff --check` completed without errors |

The ESP-IDF checkout reports `v6.0-dirty` because its file modes differ from the
tagged checkout. Its `HEAD` is exactly the v6.0 tag commit shown above; no
SleepCube build relies on an ESP-IDF source-content change.

## 3. Hardware test procedure and results

The AtomS3 Lite enumerated as Espressif USB JTAG/serial device
`44:1B:F6:F4:FE:C8`. `esptool` identified ESP32-S3 revision v0.2 with embedded
8 MB flash. Bootloader, partition table and application writes all passed hash
verification.

The board requires manual download-mode control. Hold reset for approximately
two seconds before flashing to enter download mode, then press reset once after
the flash command completes to leave download mode and start the application.
The earlier post-flash `waiting for download` observation occurred because this
second manual reset had not been performed; it is not evidence of a reset fault.
After application start, the device may re-enumerate under a new serial-device
name. A no-reset monitor captured the first button-triggered comparison sequence.

| Check | Acceptance criterion | Result |
| --- | --- | --- |
| Flash and boot | Complete flash; boot reaches `READY` without panic/reboot | PASS — all regions passed hash verification; delayed application identity/configuration log reached silent `READY` |
| Silent reset ×10 | No automatic tone, panic, watchdog, or reboot loop | PASS — ten brief resets each logged reset reason 1, silent I2S initialization and `READY`; no automatic tone or fault loop |
| Diagnostic sequence ×20 | Each retained-format run logs 286,650 frames / 1,146,600 bytes with no partial write or timeout | PASS — 21 retained-format runs completed with exact counts and no logged I2S error; the rejected MSB-mono run is not counted |
| Tone stability/quality | Stable 440 Hz and 1 kHz pitch; no dropout, crackle or distortion | PASS for first Philips-I2S run — both tones reported clear and without distortion |
| Silence/noise | No persistent objectionable noise at approximately 0.5 m in a quiet room | PASS with conservative distance deviation — silent portions reported quiet and without noise at approximately 20–30 cm |
| Transients | No objectionable software-induced click/pop at tone boundaries | PASS for first Philips-I2S run — no clicks or pops reported |

Captured sequence evidence:

```text
I (114965) m1_bringup: sequence=1 start
I (121435) m1_bringup: sequence=1 complete frames=286650 bytes=573300
```

No I2S timeout, partial write, panic or reboot was observed during that run.

Captured Philips-I2S comparison evidence:

```text
I (64495) m1_bringup: sequence=1 start
I (70955) m1_bringup: sequence=1 complete frames=286650 bytes=1146600
```

No I2S timeout, partial write, panic or reboot was observed during the comparison
run. The listener reported both tones clear and without distortion or clicks/pops,
and the silent portions quiet and without noise.

The monitor subsequently captured retained-format sequences 2 through 21. Every
run logged exactly 286,650 frames / 1,146,600 bytes. No I2S timeout, partial
write, panic or reboot was present in the captured output. The listening setup
was approximately 20–30 cm from the supplied speaker and powered from a laptop
USB port.

After adding the logging-only startup delay, the final image was reflashed with
hash verification and passed an additional clean audio regression with the exact
286,650-frame / 1,146,600-byte count.

Captured final-image application boot evidence:

```text
I (2125) m1_bringup: SleepCube Audio Controller M1 diagnostic
I (2125) m1_bringup: app=SleepCube_audio_controller version=d4089a9-dirty idf=v6.0-dirty target=esp32s3 reset_reason=1
I (2125) m1_bringup: flash=8388608 bytes (8 MiB)
I (2125) m1_bringup: i2s format=Philips-I2S stereo signed-16, duplicated mono rate=44100Hz bits=16 dma=6x256 bclk=GPIO5 lrclk=GPIO39 data=GPIO38 mclk=unused
I (2125) m1_bringup: button=GPIO41 active-low; startup playback is disabled
I (2125) audio_output: I2S output enabled in auto-clear silent state
I (2125) m1_bringup: READY: output is silent; press the button once to play the finite diagnostic sequence
```

The same complete application log through `READY` was captured on each of ten
brief reset-button repetitions. Native USB disconnect/reconnect messages between
logs are expected host-monitor behaviour, not firmware errors. No panic,
watchdog, reboot loop or automatic playback occurred.

The listening check used the 1 W/8 Ω speaker supplied with the ATOMIC Speaker
Base. This is the manufacturer's specified reference load, so the result is not
deferred pending connection of the larger TEBM speaker. The 10 W/4 Ω speaker was
not used and is not part of this evidence.

The captured initial MSB-aligned mono result was distorted. The confirmed M1
format is 44.1 kHz Philips-I2S stereo with identical left/right samples and a
reduced -24 dBFS diagnostic peak.

## 4. Conclusion and next action

M1 meets its standalone hardware exit criterion. The confirmed Philips-I2S
format produces clean, repeatable generated-tone playback on the supplied
1 W/8 Ω speaker without Waveshare involvement. Build/image checks, hash-verified
flash, complete application logging, ten silent resets, at least twenty exact-
count sequences, and the listening assessment pass.

The 20–30 cm listening distance differs from the planned approximately 0.5 m but
is conservative for detecting noise and transients. M1 does not claim storage,
decoding, product-audio fades, session timing, endurance, UART integration or
full SRS audio compliance.

**Next action:** begin M2 by selecting the content storage and codec/container
strategy, record the decision in the required ADR, then implement the smallest
standalone stored-audio decode/buffer path through the confirmed I2S output API.
