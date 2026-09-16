# ADR-0005 — Audio content storage and format

**Status:** Accepted

**Date:** 2026-09-16

**Supersedes:** None

**Superseded by:** None

## Context

M2 requires the AtomS3 Lite to read, decode and continuously present local
audio without help from the Control Controller. The ATOMIC Speaker Base already
contains a microSD slot connected to the AtomS3 Lite over SPI. The selected
format must be deterministic enough for the first 60-minute endurance gate and
must not add decoder/library risk before storage, buffering, fades and timeout
enforcement are validated.

At 44.1 kHz, mono signed 16-bit PCM uses 88,200 bytes per second. Thirty minutes
requires 158,760,000 bytes and 60 minutes requires 317,520,000 bytes, both of
which fit on the available 1 GB validation card.

## Decision

For software R00:

- content shall be stored on the ATOMIC Speaker Base microSD card;
- the card shall use a FAT16 or FAT32 filesystem;
- firmware shall never automatically format the card;
- normal playback treats content storage as read-only and shall not modify
  content files;
- the canonical content path shall be `/sleepcube/audio.wav` on the card;
- the canonical content format shall be RIFF/WAVE, uncompressed linear PCM,
  mono, signed 16-bit little-endian, at 44.1 kHz;
- the Audio Controller shall convert each mono frame to identical left/right
  samples for the confirmed Philips-I2S stereo amplifier-facing interface;
- reaching the end of valid content during an active session shall restart from
  the WAV data chunk without user intervention;
- a missing card, missing canonical file, malformed container or unsupported
  format shall prevent playback and produce an actionable, silent fault;
- a different storage or compressed-codec policy requires a superseding ADR.

The fixed path is a product-content deployment contract, not a remote
filesystem API. The Control Controller will request semantic playback and will
not select paths or operate the filesystem.

## Consequences

### Positive

- uses storage already present in the selected Speaker Base;
- requires no third-party compressed-audio decoder;
- makes decode cost and output cadence deterministic;
- preserves the proven 44.1 kHz I2S configuration;
- leaves ample capacity on the available validation card;
- makes content validation and fault reporting straightforward.

### Costs / Risks

- PCM consumes substantially more storage than MP3, AAC or Opus;
- only one canonical R00 content asset is supported;
- content preparation must produce the exact sample format;
- loop quality depends on the deployed audio asset having a suitable boundary;
- a future multi-track or storage-constrained product may require a compressed
  format and a superseding decision.

## Alternatives Considered

- MP3 on microSD — smaller assets, but introduces decoder dependency, variable
  decode load and additional failure modes before the base pipeline is proven.
- Opus or AAC on microSD — efficient, but more integration and licensing/tooling
  complexity than M2 requires.
- PCM WAV in internal flash — simple runtime, but 8 MB flash cannot hold the
  required 30-minute content and endurance asset.
- Raw PCM on microSD — simple, but lacks self-describing format fields needed to
  reject incompatible content safely.

## Verification / Implications

- parser inspection/tests shall cover RIFF/WAVE chunk traversal and rejection of
  incompatible format fields;
- hardware bring-up shall prove mount/read on the Speaker Base SPI pins and
  continuous looping through the confirmed I2S output;
- the 60-minute endurance image shall log frame, byte and loop counts without
  buffer underrun or I2S partial write;
- removal/corruption of the card or file shall fail silently and diagnostically;
- the content preparation procedure shall record the deployed file hash.

## References

- `docs/software/02_SRS.md`
- `docs/software/04_SAS.md`
- `docs/software/06_VVM.md`
- `docs/software/07_IMP.md`
- `firmware/audio/main/content_storage.c`
- `firmware/audio/main/wav_reader.c`
- M5Stack Atomic SPK Base specification and schematic
