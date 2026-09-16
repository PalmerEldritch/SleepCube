# ADR-0001: Audio P0 Playback Pipeline

## Status

Accepted

## Context

The original P0 audio PoC was defined before the final Waveshare ESP32-C6 touch-board and
external amplifier wiring were validated. The implementation has now converged on a working
hardware path:

- MP3 playback from the SD card works on the target board
- SPIFFS remains as the fallback asset store
- external mono amplifier playback works on GPIO5 / GPIO6 / GPIO7
- stop behavior requires explicit I2S TX disable to guarantee silence
- SD storage and LCD refresh share `SPI2`, so concurrent use must be serialized in software

The implementation documentation and architecture record need to reflect the validated path,
not the earlier generic bring-up assumptions.

## Decision

Use this architecture for P0:

- Preferred MP3 file location is `/sdcard/test.mp3`
- Fallback MP3 file location is `/spiffs/test.mp3`
- Helix MP3 decoder component (`esp-libhelix-mp3`)
- Fixed playback rate: 44.1 kHz
- I2S TX on the Waveshare ESP32-C6 board uses:
  - `WS/LRCLK = GPIO5`
  - `BCLK = GPIO6`
  - `DOUT = GPIO7`
- SD card shares the LCD SPI bus and uses `CS = GPIO4`
- Validated TX framing is:
  - `MSB`
  - `16-bit` sample width
  - `32-bit` slot width
- Playback stop shall disable TX instead of streaming silence
- SD refill reads shall be serialized against LVGL display activity to avoid shared-bus contention
- Optional external digital loopback monitor remains available only on boards with a supported RX path

## Consequences

### Positive

- Matches the actual working hardware path on the current P0 board.
- Keeps the software stack simple: preferred storage -> Helix decode -> PCM -> I2S.
- Provides deterministic start/stop behavior on the amplifier output.
- Allows SD-backed playback and ambient display animation to coexist on the shared SPI host.

### Negative

- Sample-rate mismatch is not corrected (no resampling yet).
- File naming and playback behavior are still static.
- SPIFFS capacity limits asset length.
- Shared-bus arbitration now depends on cooperation between the storage and display layers.

## Alternatives Considered

- Use `Philips` framing or `16-bit` slot width.
- Keep silence streaming active while playback is disabled.
- Keep the display static during SD-backed playback.
- Move directly to SD-backed playback without explicit shared-bus arbitration.

## References

- `firmware/SleepCube_P0/main/audio_player.c`
- `firmware/SleepCube_P0/main/audio_mp3.c`
- `firmware/SleepCube_P0/main/audio_i2s.c`
- `firmware/SleepCube_P0/main/audio_loopback.c`
- `docs/implementation/SC_P0_AudioPlayback.md`
