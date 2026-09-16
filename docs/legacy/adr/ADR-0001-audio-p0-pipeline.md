# ADR-0001: Audio P0 Playback Pipeline

## Status

Superseded by `docs/software/adr/ADR-0003-two-controller-software-architecture.md`.

This ADR is retained under `docs/legacy/adr/` as a historical record of the previous single-controller P0 audio architecture. It is not part of the active software R00 architecture baseline.

## Context

The original P0 audio PoC was defined before the final Waveshare ESP32-C6 touch-board and
external amplifier wiring were validated. The implementation had converged on a working
hardware path:

- MP3 playback from the SD card worked on the target board
- SPIFFS remained as the fallback asset store
- external mono amplifier playback worked on GPIO5 / GPIO6 / GPIO7
- stop behavior required explicit I2S TX disable to guarantee silence
- SD storage and LCD refresh shared `SPI2`, so concurrent use had to be serialized in software

The implementation documentation and architecture record reflected that validated P0 path.

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

- Matched the actual working hardware path on the P0 board.
- Kept the software stack simple: preferred storage -> Helix decode -> PCM -> I2S.
- Provided deterministic start/stop behavior on the amplifier output.
- Allowed SD-backed playback and ambient display animation to coexist on the shared SPI host.

### Negative

- Sample-rate mismatch was not corrected.
- File naming and playback behavior remained static.
- SPIFFS capacity limited asset length.
- Shared-bus arbitration depended on cooperation between storage and display layers.
- Audio remained tightly coupled to the Waveshare Control Controller hardware.

## Alternatives Considered

- Use `Philips` framing or `16-bit` slot width.
- Keep silence streaming active while playback is disabled.
- Keep the display static during SD-backed playback.
- Move directly to SD-backed playback without explicit shared-bus arbitration.

## Historical References

- `firmware/control/main/audio_player.c`
- `firmware/control/main/audio_mp3.c`
- `firmware/control/main/audio_i2s.c`
- `docs/legacy/implementation/SC_P0_AudioPlayback.md`
