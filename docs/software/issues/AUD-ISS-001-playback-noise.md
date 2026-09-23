# AUD-ISS-001 — Signal-correlated sleep-music noise

**Status:** Open; root cause and final acceptability not determined

**First observed:** 2026-09-18

**Updated:** 2026-09-23

**Priority:** M8/R00 release blocker until explicitly fixed, mitigated or accepted

## Symptom and impact

The Interstellar sleep-music track plays much more cleanly on the AtomS3 Lite
and ATOMIC Speaker Base than on the previous Waveshare-local audio path. It has
good clarity and bass with no reported overdrive, clicks, pops or dropouts.
Nevertheless, a higher-pitched hiss/buzz-like component is audible during music
playback. With the TEBM speaker it was barely audible at approximately 30 cm but
noticeable at approximately 10 cm. The user subsequently heard the same kind of
noise with both the supplied 1 W/8 ohm speaker and another small 3 W/4 ohm
speaker; it was more prominent relative to the music on those smaller speakers.

This makes a fault unique to the TEBM driver or its connector unlikely. It does
not distinguish the recording/downmix, digital processing, I2S, NS4168 amplifier,
power path, or a load-dependent interaction shared across speaker tests. The
three-speaker comparison is a user listening observation; no matched-level
measurement or serial capture was made for the two later speaker swaps.

## Reproduction context

- M2 normal 30-minute firmware, application version `082c7ed`;
- AtomS3 Lite plus ATOMIC Speaker Base, powered from laptop USB;
- canonical `/sleepcube/audio.wav`: 44.1 kHz, signed-16, mono PCM downmix of the
  unfiltered Interstellar stereo WAV; SHA-256
  `04295f9b56eec65350f8c3b566f04ec43409aa4b2685b37db52f763dc3a4e963`;
- TEBM observation at 50% persisted volume; a 100% comparison was also made;
- subsequent speaker-swap volume, distance and exact listening conditions were
  not recorded.

The exact mono file was preserved as `/sleepcube/audio.interstellar-mono.wav`
on the card; a matching local copy was available at
`/tmp/sleepcube-interstellar-m2-mono.wav` during the first investigation, but
temporary files must not be relied on as permanent repository evidence.

## Evidence and exclusions to date

| Test | Observation | What it supports |
| --- | --- | --- |
| Stopped output and active digital silence | Quiet | Not a constant idle or I2S-active noise floor |
| Music playback and stop | Noise tracks music and stops with it | Signal-correlated behavior |
| Same excerpt, unfiltered versus 4 kHz low-pass | No noticeable change | Noise is not resolved by removing content above 4 kHz |
| Firmware 50% versus 100% | Noise-to-music ratio changed little | Low-level digital-gain quantization is not supported as the dominant cause |
| Three speaker loads: TEBM 10 W/4 ohm, supplied 1 W/8 ohm, small 3 W/4 ohm | Noise heard on all three | TEBM-specific defect/wiring is unlikely to be the sole cause |
| 3.5 mm jack with headphones | Very high-pitched noise; music almost obscured | Invalid reference test; do not repeat |

The ATOMIC Speaker Base schematic connects the 3.5 mm jack directly to the
NS4168 differential `VOP/VON` speaker outputs, not to an independent headphone
DAC. The NS4168 is a filterless Class-D speaker amplifier. Headphone behavior
therefore cannot isolate the speaker from the upstream path, and the jack should
not be used for further listening diagnosis with these headphones. No damage
was established, but the test was stopped. See the vendor schematic linked from
the [Atomic SPK Base product page](https://docs.m5stack.com/en/atom/Atomic%20SPK%20Base)
and the local `references/datasheets/NS4168_CN_datasheet.pdf`.

The user's earlier clean laptop-headphone reference was the track, but the
record does not establish that it was the exact canonical mono conversion.
That distinction remains important.

## Next diagnostic and decision rule

1. Audition the *exact hash-matched canonical mono WAV* through a known-clean
   laptop/headphone output at a comparable passage and comfortable level.
   Record whether the same noise is present. Do not substitute the original
   stereo WAV without identifying it as a separate comparison.
2. If the mono file is noisy on the reference output, investigate the source
   recording and downmix/export chain; compare original stereo channels and a
   fresh conversion before changing embedded firmware.
3. If the mono file is clean on the reference output, use a suitable isolated,
   differential Class-D measurement method or a known-safe post-filtered test
   load to compare the Speaker Base output. Do not attach a ground-referenced
   laptop line input or ordinary grounded oscilloscope probe directly across
   `VOP/VON`. Check supply behavior and the firmware sample/gain path only with
   evidence that separates them.
4. Recheck any mitigation using the same track, firmware, power source,
   speaker, listening distance and volume. Record a before/after comparison.

M3 UART implementation may proceed. Before R00 release, this issue must be
closed by a demonstrated fix/mitigation **or** explicitly accepted as a
documented product limitation after representative listening at the intended
distance and volume. Do not silently infer closure from clean tones, active
silence, or successful UART/system tests. Record the decision and evidence here,
in `06_VVM.md`, and in the current `08_STATUS.md` handoff.

## Related evidence

- `docs/software/verification/2026-09-18_TEBM_music_followup.md`
- `docs/software/06_VVM.md`: `M2-FUP-001` and open issue `AUD-ISS-001`
