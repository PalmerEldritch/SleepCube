# TEBM Speaker and Sleep-Music Follow-up

**Date:** 2026-09-18
**Branch:** `feat/m3-uart-transport`
**Firmware:** M2 normal 30-minute image, application version `082c7ed`
**Result:** TEBM compatibility and intended-distance music quality acceptable;
minor close-field signal-correlated noise remains under investigation

## 1. Purpose and scope

This developmental follow-up was performed while the M3 UART jumper cables were
unavailable. It checks whether the 10 W/4 ohm Tectonic TEBM28C10-4/A speaker can
be used safely with the ATOMIC Speaker Base and whether the sleep-music material
that motivated the two-controller redesign is materially improved by the new
audio path.

This is not a final acoustic qualification, enclosure assessment or change to
the M2 content contract. No firmware source was changed.

## 2. Hardware and software setup

- AtomS3 Lite seated in the ATOMIC Speaker Base;
- Tectonic TEBM28C10-4/A, nominal 4 ohm/10 W, connected only to the differential
  `SPK` output using the correct two-wire connector;
- laptop USB port for power and serial monitoring;
- 1 GB FAT16 microSD card, UUID `A87B-A154`;
- normal M2 firmware using 44.1 kHz signed-16 Philips-I2S stereo with duplicated
  mono samples and the provisional -12 dBFS output cap;
- listening at approximately 30 cm for intended-use assessment and approximately
  10 cm to investigate the residual noise.

The speaker power rating is capacity, not a requested drive level. The Speaker
Base amplifier remains the limiting source power, so this test does not claim
10 W output or final maximum loudness. The driver/enclosure condition was not
qualified, so bass observations are subjective developmental evidence only.

## 3. TEBM compatibility check

The normal M2 validation content was exercised before staging the music:

| Check | Result |
| --- | --- |
| Reset/startup | Silent; no click or pop |
| 25% volume | Clear low-frequency tone, low but audible at 30 cm, fuller than the supplied speaker, no distortion |
| 50% volume | Louder and fuller in the bass, not excessively loud, no distortion or detected warmth |
| Manual stop | Clean fade to silence |
| Logged transport | 25% run stopped after 4,302,336 frames / 17,209,344 bytes and one loop; 50% run stopped after 872,960 frames / 3,491,840 bytes; no logged partial write or timeout |

The TEBM is electrically compatible with the tested Speaker Base output levels.
No thermal or audible overload indication was observed at 25% or 50%.

## 4. Music asset preparation

Four pre-existing Interstellar files in the card root were inspected and left
unchanged. The unfiltered stereo PCM WAV was selected so the test would not be
biased by the pre-existing 100 Hz high-pass variant. It was downmixed without
normalization using equal left/right contribution:

```text
pan=mono|c0=0.5*c0+0.5*c1
```

The staged full-track asset is:

- RIFF/WAVE PCM signed-16, mono, 44.1 kHz;
- duration 136.277891 seconds;
- size 12,019,788 bytes;
- peak approximately -6.0 dBFS and mean approximately -19.9 dBFS;
- SHA-256
  `04295f9b56eec65350f8c3b566f04ec43409aa4b2685b37db52f763dc3a4e963`.

## 5. Sleep-music listening result

At persisted 50% volume, startup and stopped state were silent. The track
validated as 6,009,855 frames / 136,277 ms and looped without a logged stream
fault. At approximately 30 cm the user reported:

- low but appropriate sleep-music loudness;
- good clarity and overall quality;
- good bass fullness;
- no overdriven sound, click, pop or dropout;
- a definite and substantial improvement over the former Waveshare-local audio
  setup.

A constant higher-pitched hiss/buzz-like component was barely audible at 30 cm
but noticeable when listening at approximately 10 cm. It stopped with the music.

## 6. Noise isolation

An 85-second A/B diagnostic was generated from the same converted asset:

| Interval | Content |
| --- | --- |
| 0–10 s | Digital silence while playback remained active |
| 10–40 s | Unfiltered excerpt |
| 40–45 s | Digital silence |
| 45–75 s | Identical excerpt with a two-pole 4 kHz low-pass filter |
| 75–85 s | Digital silence |

The diagnostic is signed-16 mono PCM at 44.1 kHz, 7,497,078 bytes, with SHA-256
`8a7c6d42ffb3776cf6db0cc56405586306897266cb2c8e5c7fc0966693af6bed`.

Observed results:

- all active digital-silence sections were quiet;
- no noticeable noise or music-quality difference existed between the
  unfiltered and 4 kHz low-pass passages;
- repeating the unfiltered passage at 100% increased music level by the expected
  amount, but the close-field noise remained noticeable and was only slightly
  less prominent because the bass masked it;
- the 100% comparison stopped cleanly after 1,832,960 frames / 7,331,840 bytes;
- persisted volume was returned from 100% through 25% to 50% after the test.

Offline band-level measurements of the full mono asset were:

| Approximate band | RMS level |
| --- | ---: |
| 0–500 Hz | -20.2 dBFS |
| 500–1,000 Hz | -31.4 dBFS |
| 1–2 kHz | -41.0 dBFS |
| 2–4 kHz | -52.7 dBFS |
| 4–8 kHz | -65.0 dBFS |
| 8–12 kHz | -77.1 dBFS |
| 12–20 kHz | -82.9 dBFS |

The clean active-silence result argues against a constant I2S-active, amplifier
or USB-power noise floor. The unchanged 4 kHz-filtered result and very low energy
above 4 kHz argue against inability of the TEBM to reproduce a high-frequency
band. The 12 dB gain comparison did not materially improve the noise-to-music
ratio, so low-level gain quantization is not supported as the primary cause.

The remaining noise is signal-correlated. Leading candidates are noise already
present below 4 kHz in the selected recording, an artifact of its stereo-to-mono
conversion, or content-dependent distortion later in the output chain. The test
does not distinguish those cases conclusively.

## 7. Card and handoff state

The card was left with:

- active `/sleepcube/audio.wav`: full Interstellar mono asset, SHA-256
  `04295f9b56eec65350f8c3b566f04ec43409aa4b2685b37db52f763dc3a4e963`;
- `/sleepcube/audio.interstellar-mono.wav`: identical preserved copy;
- `/sleepcube/audio.interstellar-ab.wav`: preserved A/B diagnostic, SHA-256
  `8a7c6d42ffb3776cf6db0cc56405586306897266cb2c8e5c7fc0966693af6bed`;
- `/sleepcube/audio.m2-validation.wav`: preserved M2 validation asset, SHA-256
  `e5be6d74956eec50828d22b66cddb85edc7d437d23e531254398755ede4ca4d4`.

All files were hash-verified, synced and the card safely unmounted. The Audio
Controller retains 50% in NVS. The card must be reinserted before the next boot.

## 8. Conclusion and follow-up

The 10 W/4 ohm TEBM works cleanly with the ATOMIC Speaker Base at the tested
levels, and the actual sleep music is materially better than with the previous
hardware path at the intended approximately 30 cm listening distance. The
close-field signal-correlated hiss/buzz is real but is not an M3 UART blocker.

Before final product audio tuning, audition the exact converted mono asset on a
known-clean reference output. If the noise is absent there, capture or measure
the Speaker Base output with the same passage and investigate content-dependent
amplifier/I2S behaviour and higher-precision gain/output options. Do not apply a
blanket low-pass filter: the controlled 4 kHz comparison did not improve the
observed noise.
