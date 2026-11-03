# AUD-ISS-001 — Signal-correlated sleep-music noise

**Status:** Accepted R00 limitation; retain for future improvement

**First observed:** 2026-09-18

**Updated:** 2026-11-03

**Priority:** Non-blocking after M8 reference audition; physical root cause
remains undetermined

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
| R01 debug profiles: 32/44.1/48/96 kHz, 16-/24-bit source, native/32-bit-padded slots | Clean playback without stutter/pops, but the same constant high-pitched noise level and no audible playback-character difference across modes | Tested logical sample rate, source depth and slot packing are not supported as useful mitigations; this does not prove the I2S electrical path fault-free |

The ATOMIC Speaker Base schematic connects the 3.5 mm jack directly to the
NS4168 differential `VOP/VON` speaker outputs, not to an independent headphone
DAC. The NS4168 is a filterless Class-D speaker amplifier. Headphone behavior
therefore cannot isolate the speaker from the upstream path, and the jack should
not be used for further listening diagnosis with these headphones. No damage
was established, but the test was stopped. See the vendor schematic linked from
the [Atomic SPK Base product page](https://docs.m5stack.com/en/atom/Atomic%20SPK%20Base)
and the local `references/datasheets/NS4168_CN_datasheet.pdf`.

On 2026-09-24 the exact hash-matched canonical mono file was played through the
laptop headphone output. At high headphone volume the user heard a similar,
though less noticeable, noise. Unrelated Spotify music played through the same
headphone path at high volume also exhibited comparable background noise. This
does not establish that all three noises have the same electrical cause, but it
removes the earlier assumption that a clean reference chain uniquely separates
the canonical file from the embedded path.

## R00 disposition

Accepted as a documented low-level playback limitation for R00. The decision
is supported by the complete listening set:

- on SleepCube it is barely audible at the intended approximately 30 cm
  distance and nominal sleep-music loudness, but noticeable during close
  approximately 10 cm listening;
- it occurs with three speaker loads and is therefore not unique to the TEBM
  speaker or its connector;
- stopped output and silent content are quiet, and no click, pop, dropout,
  overdrive or persistent idle noise was reported;
- the exact canonical file and unrelated streamed music both produced similar
  low-level background noise in the high-volume laptop/headphone comparison.

The evidence is sufficient for perceptual R00 acceptance, not for a hardware
root-cause claim. A future audio-quality revision may investigate amplifier
noise, power integrity and source/mastering with suitable differential
measurement equipment. The invalid Speaker Base headphone-jack method remains
prohibited.

## Future diagnostic path

1. Do not continue sweeping logical I2S sample rate, source depth or slot width
   without new evidence; the controlled R01 listening comparison found no
   audible difference across those variables.
2. If further improvement is prioritized, investigate the source recording and
   downmix/export chain; compare original stereo channels and a fresh conversion
   before changing embedded firmware.
3. Use a suitable isolated,
   differential Class-D measurement method or a known-safe post-filtered test
   load to compare the Speaker Base output. Do not attach a ground-referenced
   laptop line input or ordinary grounded oscilloscope probe directly across
   `VOP/VON`. Check supply behavior and the firmware sample/gain path only with
   evidence that separates them.
4. Recheck any mitigation using the same track, firmware, power source,
   speaker, listening distance and volume. Record a before/after comparison.

This acceptance must remain visible in release documentation. Clean tones,
active silence or successful UART/system tests alone would not have closed the
issue; the representative intended-track and exact-reference observations are
the decision evidence.

## Related evidence

- `docs/software/verification/2026-09-18_TEBM_music_followup.md`
- `docs/software/verification/2026-11-03_M10_i2s_debug_profiles.md`
- `docs/software/06_VVM.md`: `M2-FUP-001` and open issue `AUD-ISS-001`
