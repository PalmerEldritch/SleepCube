# M2 Standalone Audio Verification Record

**Dates:** 2026-09-16 (implementation/card preparation), 2026-09-17
(hardware verification)

**Branch:** `feat/m2-standalone-audio`

**Milestone status:** In progress; normal 30-minute hardware gate passes,
60-minute endurance playback and content-fault injection remain pending

## 1. Intended hardware

- M5Stack AtomS3 Lite seated in ATOMIC Speaker Base;
- supplied 1 W/8 Ω speaker on the Speaker Base `SPK` connector;
- available 1 GB FAT16 card (UUID `A87B-A154`), with previous content backed up
  before M2;
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

The generated 60-second validation asset was copied without removing or changing
the card's existing files. The deployed file was then read back and verified:

- path: `/sleepcube/audio.wav`;
- size: 5,292,044 bytes;
- detected format: RIFF/WAVE, Microsoft PCM, 16-bit mono, 44.1 kHz;
- SHA-256: `e5be6d74956eec50828d22b66cddb85edc7d437d23e531254398755ede4ca4d4`;
- byte comparison against the generated source: PASS;
- free space after deployment: approximately 888.9 MiB.

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
| FAT16 card content staging/read-back | PASS | Canonical 60-second asset matches its generated source byte-for-byte and existing card files remain present |
| Repository whitespace check | PASS | `git diff --check` reports no errors |

The installed ESP-IDF worktree identifies itself as `v6.0-dirty` due to
previously documented file-mode differences; its `HEAD` remains the required
commit and no ESP-IDF source modification is required by M2.

Both images were rebuilt at repository commit `61b3b26` before flashing. This
commit changes only `.vscode/settings.json`; the M2 firmware source baseline is
the preceding implementation commit `10c38d8`. Each rebuilt image is 314,352
bytes and reports application version `61b3b26` at boot.

## 4. Normal-session hardware evidence

Hardware used on 2026-09-17 matched section 1. Power was supplied by the laptop
USB port. Listening used the supplied 1 W/8 ohm speaker; the TEBM speaker and
Waveshare controller were not connected. M2 listening distance was not recorded
and remains an evidence limitation.

### 4.1 Boot, storage and content

The normal image was hash-verified during flash and produced a complete captured
application log:

- application `SleepCube_audio_controller`, version `61b3b26`, ESP32-S3, 8 MiB
  flash;
- FAT16 card mounted over SPI2 with GPIO7 clock, GPIO6 MOSI, GPIO8 MISO and
  fixed-low card select;
- reported card capacity 1,007,681,536 bytes;
- `/sdcard/sleepcube/audio.wav` validated as PCM mono signed-16 at 44.1 kHz,
  2,646,000 frames and 60,000 ms;
- I2S entered its auto-clear silent state and the controller reached `STOPPED`
  and `READY` without automatic playback.

The ESP-IDF SD probe logged unsupported CMD5 on the initial boot and CMD52 CRC
plus unsupported CMD5 after reset. In both cases initialization continued
immediately to a successful mount and full content validation. No runtime card
or read error followed.

### 4.2 Playback, looping, fades and volume

The first 50% run entered `PLAYING` after its one-second fade. One complete
content loop was observed with no audible dropout, gap, click or pop at the loop
boundary. Manual stop completed its one-second fade and reported `STOPPED` with
5,171,200 frames / 20,684,800 I2S bytes and no partial write or timeout.

All four bounded volume levels were exercised. The user reported clear playback
and clean start/stop transitions at every level:

| Level | Logged Q15 gain | Listening result |
| --- | ---: | --- |
| 25% | 512 | Very noticeably quieter; no playback or transition artifact |
| 50% | 2,048 | Clean initial playback, loop boundary and manual fade |
| 75% | 4,608 | Noticeably louder than 50%; no distortion, noise, click or pop |
| 100% | 8,192 | Louder than 75%; no distortion, noise, click or pop |

The displayed 100% level remained subject to the implemented provisional
-12 dBFS cap. The test content was heard as a low-frequency tone. No persistent
noise was heard while stopped.

### 4.3 Persistence and reset

Volume was changed from 50% to 75%, logged as persisted, and the controller was
reset. The next boot loaded 75% from NVS, remounted and revalidated the card,
then reached silent `STOPPED`/`READY` without automatic playback. Volume cycling
was subsequently completed through 100% and 25% and returned to a persisted 50%
before the timer run.

### 4.4 Normal 30-minute timeout

The normal image started a 1,800-second session at 50%. Heartbeats remained
monotonic through the run. It completed 29 full content loops, requested
`session-timeout` five seconds before the finite deadline, completed loop 30
during the fade, and reached `STOPPED` at the deadline with:

- 79,381,504 frames;
- 317,526,016 I2S bytes;
- 30 content loops;
- no underrun, short/partial I2S write, storage error, panic, watchdog or reboot.

The user reported that the final five-second fade was smooth and that the
speaker remained silent afterward.

## 5. Endurance checkpoint and pending evidence

The separate endurance image was rebuilt, hash-verified during flash and booted
successfully. Its captured log identified `M2 60-minute endurance`, restored the
persisted 50% volume, mounted and validated the same card/content, reported a
3,600-second session, and reached silent `STOPPED`/`READY`. The serial monitor
was then closed and playback was deliberately **not started** when the session
was paused. No endurance playback result is claimed.

Still pending before M2 completion:

- run the installed endurance image for its full 60-minute finite session and
  verify heartbeats, exact final state, absence of underrun/partial write/panic/
  watchdog/reboot, and listening quality;
- exercise missing-file/card and malformed or unsupported WAV fault paths,
  confirming actionable diagnostics and silent `FAULT` behaviour;
- record the listening distance for the remaining M2 run;
- after test-only endurance/fault work, restore the normal image and perform a
  short card/playback regression before handing the Audio Controller to M3.
