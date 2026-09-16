# M2 Standalone Audio Verification Record

**Date:** 2026-09-16

**Branch:** `feat/m2-standalone-audio`

**Milestone status:** In progress; build evidence passes, hardware evidence pending

## 1. Intended hardware

- M5Stack AtomS3 Lite seated in ATOMIC Speaker Base;
- supplied 1 W/8 Ω speaker on the Speaker Base `SPK` connector;
- available 1 GB card reported by the host as `vfat`, with previous content
  backed up before M2;
- laptop USB port for power, flashing and serial monitoring;
- no Waveshare controller, UART wiring or TEBM 10 W/4 Ω speaker.

## 2. Frozen M2 content contract

ADR-0005 selects a FAT16/FAT32 microSD card and
`/sleepcube/audio.wav`, containing RIFF/WAVE linear PCM, mono, signed 16-bit
little-endian samples at 44.1 kHz. Runtime content use is read-only and firmware
does not automatically format the card.

The deterministic generator produced a two-second structural test file:

- size: 176,444 bytes;
- detected format: RIFF/WAVE, Microsoft PCM, 16-bit mono, 44.1 kHz;
- SHA-256: `02f05343ebb010d00da938825d6046b07c488fea6a16685b69dd31294708c102`.

The actual card-deployed validation asset and hash remain to be recorded.

## 3. Build evidence

Toolchain:

- ESP-IDF v6.0 commit `662a3be354759d9487bf4b1a629fadb766cb1800`;
- target ESP32-S3;
- configured flash size 8 MB.

Results:

| Configuration | Result | Image |
| --- | --- | --- |
| Normal finite 30-minute session | PASS | 314,352 bytes; checksum and validation hash valid; 70% of 1 MB app partition free |
| Finite 60-minute endurance image | PASS | 314,352 bytes; `CONFIG_SC_M2_ENDURANCE_BUILD=y`; 70% of 1 MB app partition free |
| Deterministic WAV generator smoke test | PASS | Correct RIFF/WAVE PCM mono signed-16 44.1 kHz output |
| Repository whitespace check | PASS | `git diff --check` reports no errors |

The installed ESP-IDF worktree identifies itself as `v6.0-dirty` due to
previously documented file-mode differences; its `HEAD` remains the required
commit and no ESP-IDF source modification is required by M2.

## 4. Hardware evidence pending

- identify the card and exact mounted FAT variant on target;
- validate canonical file discovery and rejected missing/unsupported content;
- verify silent startup and deterministic `READY` state;
- verify start/stop fades and loop boundary listening quality;
- exercise 25%, 50%, 75% and 100% bounded volume transitions;
- change volume and reset/power-cycle to prove NVS restoration;
- verify the normal 30-minute fade reaches `STOPPED` at its deadline;
- run the separate 60-minute endurance image without underrun, partial write,
  panic, watchdog or reboot;
- record listening distance, power source, content hash and audible limitations.

No M2 hardware or listening claim is made by this record yet.
