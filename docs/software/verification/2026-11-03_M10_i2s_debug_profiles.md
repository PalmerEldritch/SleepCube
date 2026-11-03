# M10 Touch-Debug I2S Profile Extension — Software Evidence

**Date:** 2026-11-03
**Branch:** `feat/r01-touch-debug-ui`
**Scope:** host/build evidence, matched flash/boot smoke, stopped-state
source-cycle validation and user-operated rate/depth/slot listening comparison

## Implemented scope

- A3 now cycles canonical Music; controlled 44.1/16, 44.1/24, 48/16, 48/24,
  32/16 and 96/16 mono WAV profiles; locally generated 44.1 kHz C5; and then
  Music again.
- A4 independently toggles native-width and 32-bit-padded I2S slots, producing
  16/16, 16/32, 24/24 and 24/32 comparisons.
- The Audio owner validates exact file rate, depth, channel count, PCM encoding
  and data alignment before applying a selection. Classic PCM and
  WAVE_FORMAT_EXTENSIBLE integer-PCM headers are accepted. Failure retains the
  last applied profile while the Control cycle cursor advances, preventing one
  rejected asset from trapping A3 on the same selection.
- WAV samples use a Q31 decode/gain path so 24-bit precision is retained until
  final duplicated-stereo packing. The existing gain law, fades and finite
  Audio-owned deadline remain in force.
- Runtime I2S reconfiguration remains Philips-I2S master output on GPIO5 BCLK,
  GPIO39 LRCLK/WS and GPIO38 DATA. Native 24-bit mode uses the required 384×
  internal MCLK multiple and a DMA frame count divisible by three; no external
  MCLK pin is enabled.
- B4 restores canonical music, C5 and native slots. Profile/packing selections
  remain volatile, and normal profiles compile out the test catalog and grid.

## Host verification

All suites were compiled as strict C11 with `-Wall -Wextra -Werror -pedantic`
and UndefinedBehaviorSanitizer.

| Check | Result |
| --- | --- |
| Semantic protocol suite | PASS — source values 0–7, packing command, capability gating, invalid values and status encoding/decoding |
| Audio profile-catalog suite | PASS — all exact paths, rates/depths, generated profile and invalid lookup |
| PCM conversion/packing suite | PASS — signed 16-/24-bit LE to Q31, quantization, stereo duplication and bounded destination |
| Generated-tone suite | PASS — C3–C8 frequency table, bounds and deterministic restart; touch grid uses C5 |
| WAV-format suite | PASS — classic PCM16 and extensible integer PCM24 accepted; extensible float and malformed/truncated extensions rejected |
| Repository whitespace check | PASS — `git diff --check` |

The host sources were built and executed with these command forms from the
repository root (all executables were placed under
`/tmp/sleepcube-i2s-host-tests/`):

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/shared/protocol firmware/shared/protocol/sc_frame.c \
  firmware/shared/protocol/sc_audio_protocol.c \
  firmware/shared/protocol/tests/test_sc_audio_protocol.c \
  -o /tmp/sleepcube-i2s-host-tests/test_sc_audio_protocol

cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/audio/main -Ifirmware/shared/protocol \
  firmware/audio/main/audio_test_profiles.c \
  firmware/audio/main/tests/test_audio_test_profiles.c \
  -o /tmp/sleepcube-i2s-host-tests/test_audio_test_profiles

cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/audio/main firmware/audio/main/pcm_format.c \
  firmware/audio/main/tests/test_pcm_format.c \
  -o /tmp/sleepcube-i2s-host-tests/test_pcm_format

cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/audio/main -Ifirmware/shared/protocol \
  -DSC_TONE_SAMPLE_RATE_HZ=44100 firmware/audio/main/tone_generator.c \
  firmware/audio/main/tests/test_tone_generator.c \
  -o /tmp/sleepcube-i2s-host-tests/test_tone_generator

cc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
  -Ifirmware/audio/main firmware/audio/main/wav_format.c \
  firmware/audio/main/tests/test_wav_format.c \
  -o /tmp/sleepcube-i2s-host-tests/test_wav_format
```

## ESP-IDF v6.0 builds

| Profile | Result | Application size | SHA-256 |
| --- | --- | ---: | --- |
| ESP32-C6 matched debug Control, `R01-touch-debug` | PASS | 594,400 bytes | `1e73f82f9b4533ff0c0299568fc5f969d42421d6317324f9563f4328d4823973` |
| ESP32-S3 matched debug Audio, `R01-touch-debug-audio` | PASS | 346,880 bytes | `f5f3b64e397d3626c1f800b9aa7a89cf1ca8cea7d0c3c3473373e931856703e5` |
| ESP32-C6 normal R01 Control, touch grid unset | PASS | 597,920 bytes | `46b6a240742633733a5ea35deb27ca18a1e1c6ba225d789d57e468447cbea986` |
| ESP32-S3 normal Audio, debug Audio unset | PASS | 343,840 bytes | `caffea332df5e778fdb03f03e6da3b0d3fb1064acadcd0ed93aa049f6774f156` |

Build directories and generated SDK configuration files remain outside the
repository under `/tmp`. The debug configuration files select the intended
identities and feature flags; both normal configurations leave their touch-
debug option unset.

The matched profiles use the README build commands with external directories
`/tmp/sleepcube-i2s-debug-control-build` and
`/tmp/sleepcube-i2s-debug-audio-build`; the normal regressions use the matching
`*-normal-*` paths. Both Audio configurations were rebuilt from `fullclean`;
Control debug was clean-built after the protocol/UI changes and normal Control
was created through `set-target`, which performs `fullclean`.

## Matched target flash and boot smoke

The laptop enumerated the Waveshare ESP32-C6 as `/dev/ttyACM0`, USB identity
`AC:EB:E6:1E:67:C4`, and the Atom ESP32-S3 as `/dev/ttyACM1`, USB identity
`44:1B:F6:F4:FE:C8`. These identities matched the earlier hardware records.

Both exact debug application artifacts in the table above were flashed on 2026-11-03.
Every bootloader, partition-table and application write passed esptool data-hash
verification. Short reset-and-monitor captures then established:

- Audio booted `R01-touch-debug-audio`, mounted the approximately 1 GB FAT card
  read-only-by-policy at `/sdcard`, validated canonical
  `/sdcard/sleepcube/audio.wav`, enabled silent 44.1 kHz 16/16 Philips-I2S and
  reached `STOPPED` at persisted 50% volume.
- Control booted `R01-touch-debug`, initialized GPIO8 at requested 3,000 Hz /
  actual 3,001 Hz with 13-bit resolution, completed display initialization,
  negotiated Audio capability mask `0x000f` and reconciled authoritative
  `STOPPED`, 50% volume, zero fault.
- Audio received the Control `HELLO` and continuing `GET_STATUS` traffic. No
  playback was started by either reset.

## Profile-cycle defect and correction

The first interactive pass selected 44.1/16 successfully but rejected the next
44.1/24 file as unsupported. A3 then appeared stuck because it derived every
next selection from the last Audio-confirmed source, and the original cycle did
not include a path back to Music after leaving it.

The on-card 24-bit assets use the standard WAVE_FORMAT_EXTENSIBLE integer-PCM
header rather than the classic PCM tag handled by the initial reader. The
reader now parses both representations and retains strict checks of mono PCM,
sample rate, container bits, valid bits, byte rate, block alignment and data
alignment. A separate volatile Control cursor advances when a source request is
queued, independent of whether Audio later accepts the asset. Music was added
to the wrap sequence. Both corrected artifacts in the build table were then
flashed with successful write/hash verification.

With playback stopped, target logs subsequently confirmed all actual copied
assets were accepted and configured:

| Source | Frames | Duration | Native I2S data/slot |
| --- | ---: | ---: | --- |
| 44.1 kHz / 16-bit | 529,200 | 12,000 ms | 16/16 |
| 44.1 kHz / 24-bit | 529,200 | 12,000 ms | 24/24 |
| 48 kHz / 16-bit | 576,000 | 12,000 ms | 16/16 |
| 48 kHz / 24-bit | 576,000 | 12,000 ms | 24/24 |
| 32 kHz / 16-bit | 384,000 | 12,000 ms | 16/16 |
| 96 kHz / 16-bit | 1,152,000 | 12,000 ms | 16/16 |

Continuing A3 also selected generated C5, returned to canonical Music, and then
selected 44.1/16 again. This verifies the complete stopped-state wrap and that
the prior one-way/stuck behavior is corrected. It does not constitute an
audible playback or noise-character result.

## Interactive listening result

The user subsequently exercised every source mode and the available I2S
configuration modes on the corrected matched pair. All sources selected and
played as expected; no further source-cycle or file-acceptance issue occurred.

The listening result was negative for the intended noise discrimination:

- no audible change was detected between the tested sample rates, 16-/24-bit
  source depths or native/32-bit-padded slot configurations;
- the background retained the same constant high-pitched noise in every tested
  configuration and at the same perceived level as before the experiment;
- playback remained clean, with no stutter or pops in any tested mode.

This is user listening evidence rather than an electrical or captured-audio
measurement. It does not prove that I2S is electrically fault-free, but it
shows that changing the tested logical rate, depth and slot packing is not an
audible mitigation. The constant high-pitched component remains tracked by
`AUD-ISS-001`.

## Open target verification

- Verify missing/mismatched-file rejection retains the prior profile, STOP
  keeps the selected I2S output configured but silent, the finite deadline is
  not extended, and B4 restores canonical music/C5/native slots.
- Repeat the existing debug lighting and normal-mode regression checks. Record
  Audio task stack margin on target because the Q31 and 24-bit packing buffers
  increased the playback task stack allocation from 6,144 to 8,192 bytes.
No electrical I2S timing, analog output or captured-audio measurement is
claimed by this report.
