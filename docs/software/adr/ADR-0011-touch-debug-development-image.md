# ADR-0011 — Touch Debug Development Image

**Status:** Accepted  
**Date:** 2026-10-31  
**Supersedes:** None  
**Superseded by:** None

## Context

COB frequency, exact duty and audio checks require repeatable controls while
the assembled prototype is powered independently of the development laptop.
The USB serial console proved dependent on terminal line-ending and device
enumeration behavior. The normal ambient GUI and eventual R01 release image
must remain unchanged.

## Decision

A matched pair of development-only Control and Audio profiles shall provide a
touch debug mode. Five rest-view taps with no inter-tap gap above 350 ms enter a
full-screen 2-by-4 grid. One through four taps after the timeout retain one
normal playback-toggle action.

The grid controls PLAY/STOP, persistent volume in wrapping 10% steps, a
volatile light-enable latch, exact physical COB duty in wrapping 10% steps,
music/generated-sine source, 1–25 kHz PWM in wrapping 1 kHz steps, C3–C8 tone
selection and explicit exit. Audio controls are blue, lighting controls green
and exit red. Debug mode fixes display backlight at 70% and disables COB
ambient and PLAY/STOP effects. Exact duty bypasses the normal 5–80% product
mapping, but the separate light-enable latch can still force zero output.

The Audio development image generates the selected sine directly at 44.1 kHz.
Its raw peak is -12 dBFS and passes through the retained -12 dBFS output cap,
giving a maximum -24 dBFS test tone. Source/note changes during playback use a
one-second fade-out and fade-in without replacing or extending the existing
finite Audio-owned session deadline.

Semantic version 1 gains optional capability bit 3 and development commands
`SET_SOURCE`/`SET_TONE_NOTE`. Normal Audio images do not advertise the
capability and return unsupported for those commands. Exiting restores music,
C5, 3 kHz, light enabled and normal effects; restoration is retried after an
Audio reconnection. Exact duty, frequency, source and note are volatile.

The profiles are `sdkconfig.r01.touch-debug.defaults` and
`sdkconfig.touch-debug.defaults`. Normal/release profiles compile out the touch
grid and generated-tone source.

## Consequences

### Positive

- Hardware variables can be controlled without a host terminal.
- Exact duty is clearly separated from persisted product brightness.
- Normal GUI layout and release-image behavior remain unchanged.
- Tone generation does not require another SD-card asset.

### Costs / Risks

- Both development images must be flashed as a matched pair for source controls.
- Five-tap recognition defers the normal single-tap action by up to 350 ms in
  this development image.
- The optional semantic catalog and status flags require ICD and host-test
  coverage even though they are absent from the release UX.

## Alternatives Considered

- Continue with USB serial only — rejected because it does not meet the
  standalone assembled-prototype workflow.
- Store one WAV per tone — rejected because generated sine output is smaller,
  deterministic and independent of content-card preparation.
- Put the debug grid in the final image — rejected because this is a hardware
  evaluation seam, not a product interaction.

## Verification / Implications

- Host tests shall cover optional protocol commands/status flags and generated
  tone frequencies, bounds and deterministic restart.
- Both development profiles shall clean-build under ESP-IDF v6.0.
- Target checks remain required for five-tap behavior, every grid control,
  exact duty/frequency, effect suppression, tone level/notes, source-switch
  fades, timeout retention, exit restoration and normal-mode recovery.

## References

- `docs/software/03_UX.md`
- `docs/software/04_SAS.md`
- `docs/software/05_ICD.md`
- `firmware/control/sdkconfig.r01.touch-debug.defaults`
- `firmware/audio/sdkconfig.touch-debug.defaults`
