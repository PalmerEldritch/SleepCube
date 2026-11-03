# M10 Touch-Debug Development Image — Software Evidence

**Date:** 2026-10-31  
**Branch:** `feat/r01-touch-debug-ui`  
**Scope:** implementation/build plus initial matched flash and boot evidence;
interactive touch/PWM/audio verification remains open

## Implemented scope

- Five-tap development-only touch entry and high-contrast 2-by-4 grid.
- Exact volatile duty (0–100%), independent light enable, volatile 1–25 kHz
  PWM, fixed debug backlight and lighting-effect suppression.
- Volume/PLAY controls through the existing Control facade.
- Optional semantic capability, source and C3–C8 note commands/status.
- Local 44.1 kHz generated sine with -24 dBFS maximum after retained gain cap.
- Active source/note changes use one-second down/up ramps without replacing the
  finite playback deadline.
- Exit clears debug lighting controls and restores music/C5, including deferred
  restoration after Audio reconnect.

## Verification performed

| Check | Result |
| --- | --- |
| Strict C11/UBSan semantic protocol host test | PASS — includes debug capability, source/note commands, invalid values and status encoding |
| Strict C11/UBSan generated-tone host test | PASS — C3..C8 frequencies, octave progression, sample bounds and deterministic restart |
| ESP-IDF v6.0 ESP32-C6 touch-debug Control clean build | PASS — identity `R01-touch-debug`, 593,680-byte application |
| ESP-IDF v6.0 ESP32-S3 touch-debug Audio clean build | PASS — identity `R01-touch-debug-audio`, 343,488-byte application |
| ESP-IDF v6.0 normal-profile regression builds | PASS — Control 597,680 bytes with `CONFIG_SC_ENABLE_TOUCH_DEBUG_MODE` unset; Audio 342,160 bytes with `CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO` unset |
| Exact matched target flash/write verification | PASS — Control application SHA-256 `0f989121ff1719e07e1490ff10c3248d0a45d4f06d6d6694dea9699f7daf21df` flashed to ESP32-C6 base MAC `ac:eb:e6:1e:67:c4`; Audio application SHA-256 `101ce82c3fb37ec373e2a9f6af15bc3aa156bc4c34e022c7927090a4f0fef36a` flashed to ESP32-S3 MAC `44:1b:f6:f4:fe:c8`; every bootloader, partition-table and application write passed hash verification |
| Matched boot and UART smoke | PASS — Control booted `R01-touch-debug`, initialized GPIO8 at requested 3 kHz/actual 3001 Hz and synchronized with capability mask `0x000f`; Audio booted `R01-touch-debug-audio`, mounted the SD card, validated the canonical WAV and reached silent `STOPPED` at persisted 50% volume |

Build directories and generated SDK configuration files were kept outside the
repository under `/tmp`. The exact rebuilt artifacts above were then flashed on
2026-10-31. No grid interaction, generated-tone listening, or powered PWM
measurement is claimed by this initial smoke check.

## Open target verification

- Five-tap timing, incomplete-tap normal action and long-press regression.
- Every button, wrapping boundary, disabled-audio state and high-visibility
  presentation.
- Exact 0–100% duty and 1–25 kHz measurements; B1/B2 independence.
- Absence of ambient and PLAY/STOP duty effects in debug mode.
- C3–C8 generated audio, level, noise/distortion and music/tone transition fades.
- Existing deadline retention across source/note changes.
- B4 restoration of normal effects, 3 kHz, light enabled, music/C5 and normal UI.

## Next task

With external 12 V off, validate entry/control semantics and GPIO8 safety, then
energize the existing proven prototype power topology for PWM and audio
observations.
