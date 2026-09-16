# SleepCube P0 Audio Investigation Notes

## Purpose

Capture the audio diagnostics, experimental changes, and observed behavior from the
pre-hardware-refresh investigation phase. This document is intentionally work-in-progress
 oriented and complements `SC_P0_AudioPlayback.md`.

## Current Status

- Audio playback, start/stop fade behavior, and runtime volume control are functional.
- SD-backed asset playback is working again after SD mount robustness fixes.
- Basic output chain is capable of clean sine tones and sweeps.
- Real program material still shows audible harshness with the currently installed low-cost driver.
- Audio tuning work is paused until the replacement driver hardware is available.

## Implemented Diagnostic Capabilities

| Capability | Status | Notes |
| --- | --- | --- |
| Runtime serial audio console | Implemented | Development-only CLI over serial monitor |
| Diagnostic tone generator | Implemented | Fixed tones and sweep available at runtime |
| Runtime HPF control | Implemented | Tunable cutoff and stage count |
| Runtime MP3 source selection | Implemented | SPIFFS / SD / auto |
| Runtime SD track selection | Implemented | Lists SD root entries and selects by index/path |
| Runtime MP3 mix mode | Implemented | `stereo`, `mono`, `left`, `right` |
| Runtime MP3 pre-gain | Implemented | `mp3gain` in dB attenuation |
| Runtime MP3 limiter | Implemented | Kept for diagnostics, not recommended as current solution |
| Runtime MP3 EQ | Implemented | Kept for diagnostics, not recommended as current solution |
| WAV playback path | Implemented | 16-bit PCM WAV for A/B comparison against MP3 |

## Key Test Sequence and Findings

| Test Area | Observation | Insight |
| --- | --- | --- |
| Start/stop fade | Fade-in/out behavior subjectively good | Control path is acceptable as baseline |
| Volume control | Raising and lowering volume works correctly | Runtime gain control path is functional |
| Tone generator (`500 Hz`, sweep) | Clean audio without clicks, rumble, or obvious harshness | Basic I2S, amplifier drive, and speaker path are not fundamentally broken |
| HPF at `150 Hz`, `300 Hz`, `400 Hz`, `600 Hz` | Little benefit; higher settings worsened audio | Distortion is not primarily caused by excess low-frequency content |
| SD versus SPIFFS MP3 | Sounded effectively the same | Shared SD/LCD SPI bus contention is not the primary audible defect during steady playback |
| MP3 diagnostics counters | `dec=0`, `sync=0`, `rate=0` during testing | Audible issue is not explained by decode failure, sync loss, or sample-rate mismatch |
| Stereo versus mono MP3 content | Mono helped slightly on one track, but `mp3mix` modes made no meaningful difference | Stereo-to-mono collapse is not the dominant remaining issue |
| MP3 pre-gain | `mp3gain 6` gave the best overall result so far | Headroom / gain structure is a real contributor |
| MP3 limiter | Reduced neither harshness nor clicks cleanly; added extra distortion | Current limiter approach is not suitable |
| MP3 EQ / presence-cut / LPF | Added digital-sounding artifacts and stutter without solving harshness | Current software EQ approach is not suitable on this path |
| WAV versus MP3 | WAV sounded somewhat cleaner and less hot on speech, but not dramatically different | MP3 path contributes to harshness, but the output hardware path is still a major factor |

## SD Card Bring-Up Findings

| Item | Observation | Action Taken |
| --- | --- | --- |
| Initial SD mount | Card frequently failed early initialization with `ESP_ERR_INVALID_RESPONSE` | Added pin preparation, mount retry loop, and fallback host frequencies |
| Runtime remount | Live remount while UI was active caused instability | Avoided depending on runtime remount for normal behavior |
| Post-mount path probing | Immediate SD `stat()` probing caused boot panic after recovered mount | Startup path was changed to avoid aggressive SD default-file probing |
| Current status | SD initializes and tracks can be listed/read | Good enough for continued diagnostics |

## Audio Quality Interpretation

| Topic | Current Interpretation |
| --- | --- |
| Low bass overload | Unlikely to be the primary cause |
| MP3 decode correctness | Not the primary cause |
| SD read glitches | Not the primary cause of the remaining artifact |
| Peak headroom | Clearly part of the problem |
| Midrange / voice harshness | Still present even after peak control; likely tied to driver / amplifier / acoustic behavior |
| Hardware quality ceiling | Strongly suspected with the currently installed low-cost driver |

## Recommended Baseline Until New Driver Arrives

| Setting | Current Recommendation | Rationale |
| --- | --- | --- |
| Playback format for evaluation | Prefer WAV where practical | Slightly cleaner than MP3 during testing |
| MP3 fallback baseline | `mp3gain 6`, limiter off, EQ off | Best current MP3 compromise found |
| HPF | Leave off unless specific bass test demands it | Did not improve the actual issue |
| Limiter | Leave off | Added audible artifacts |
| EQ | Leave off | Added audible artifacts |

## Known Gaps To Revisit After Hardware Refresh

| Area | Next Step After New Driver Arrives |
| --- | --- |
| Driver / speaker path | Re-run tone, sweep, MP3, and WAV comparison tests |
| Headroom | Re-evaluate best pre-gain with the new hardware |
| Speech harshness | Repeat talk-track comparison first; this remains the clearest indicator |
| Ambient material noise floor | Re-test sparse low-level ambient content, since it exposes artifacts most clearly |
| Final format choice | Decide between WAV and compressed assets based on new hardware results |
| UI / ambient screen polish | Resume after audio is considered acceptable |
