# ADR-0001: Audio P0 Playback Pipeline

## Status

Accepted

## Context

The first PoC must validate the digital audio path before amplifier hardware is available.
The firmware should prove file -> decode -> PCM -> I2S behavior with minimal complexity.

## Decision

Use this architecture for P0:

- MP3 file stored in SPIFFS partition (`/spiffs/test.mp3`)
- Helix MP3 decoder component (`esp-libhelix-mp3`)
- Fixed I2S output format: 44.1 kHz, 16-bit, stereo
- Optional external digital loopback monitor controlled by `CONFIG_SC_LOOPBACK_ENABLE`

## Consequences

### Positive

- Fast bring-up and reproducible verification without analog hardware.
- Small code footprint and straightforward debug path.
- Loopback telemetry provides objective pass/fail signals in logs.

### Negative

- Sample-rate mismatch is not corrected (no resampling yet).
- File path and playback behavior are static.
- No user playback controls in current PoC.

## Alternatives Considered

- Start with analog amplifier validation first.
- Use a different decoder stack (for example ADF-based pipeline).
- Stream from external media before proving core path.

## References

- `firmware/SleepCube_P0/main/audio_player.c`
- `firmware/SleepCube_P0/main/audio_mp3.c`
- `firmware/SleepCube_P0/main/audio_i2s.c`
- `firmware/SleepCube_P0/main/audio_loopback.c`
