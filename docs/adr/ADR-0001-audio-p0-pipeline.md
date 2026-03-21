# ADR-0001: Audio P0 Playback Pipeline

## Status

Accepted

## Context

The original P0 audio PoC was defined before the final Waveshare ESP32-C6 touch-board and
external amplifier wiring were validated. The implementation has now converged on a working
hardware path:

- MP3 playback from SPIFFS works on the target board
- external mono amplifier playback works on GPIO5 / GPIO6 / GPIO7
- stop behavior requires explicit I2S TX disable to guarantee silence

The implementation documentation and architecture record need to reflect the validated path,
not the earlier generic bring-up assumptions.

## Decision

Use this architecture for P0:

- MP3 file stored in SPIFFS partition (`/spiffs/test.mp3`)
- Helix MP3 decoder component (`esp-libhelix-mp3`)
- Fixed playback rate: 44.1 kHz
- I2S TX on the Waveshare ESP32-C6 board uses:
  - `WS/LRCLK = GPIO5`
  - `BCLK = GPIO6`
  - `DOUT = GPIO7`
- Validated TX framing is:
  - `MSB`
  - `16-bit` sample width
  - `32-bit` slot width
- Playback stop shall disable TX instead of streaming silence
- Optional external digital loopback monitor remains available only on boards with a supported RX path

## Consequences

### Positive

- Matches the actual working hardware path on the current P0 board.
- Keeps the software stack simple: SPIFFS -> Helix decode -> PCM -> I2S.
- Provides deterministic start/stop behavior on the amplifier output.

### Negative

- Sample-rate mismatch is not corrected (no resampling yet).
- File path and playback behavior are still static.
- SPIFFS capacity limits asset length.
- SD-backed playback is deferred.

## Alternatives Considered

- Use `Philips` framing or `16-bit` slot width.
- Keep silence streaming active while playback is disabled.
- Move directly to SD-backed playback before stabilizing the internal-flash playback path.

## References

- `firmware/SleepCube_P0/main/audio_player.c`
- `firmware/SleepCube_P0/main/audio_mp3.c`
- `firmware/SleepCube_P0/main/audio_i2s.c`
- `firmware/SleepCube_P0/main/audio_loopback.c`
- `docs/implementation/SC_P0_AudioPlayback.md`
