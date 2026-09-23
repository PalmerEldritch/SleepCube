# M7 UX and lighting integration refinement — verification record

**Date:** 2026-09-23
**Status:** PASS — all non-deferred UX requirements have passing implementation,
target and/or retained lower-layer evidence.

## Scope

M7 verifies the retained touch-first UI and low-light presentation against the
distributed Control/Audio state model. It closes one rest-gesture fall-through
defect and one physical low-output LED-strip defect without changing controller
ownership, the UART protocol or Audio firmware.

## Implementation

- `727285d` makes significant rest-view movement cancel the complete gesture.
  Releasing that gesture cannot emit the short-tap audio-toggle event. It also
  adds focused trace markers for press, cancellation, click suppression, long
  press, mode changes and application event dispatch.
- `sdkconfig.m7.defaults` defines the normal ESP32-C6/8 MB product with remote
  Audio, UI/lighting and a finite 1,800-second PLAY duration.
  `sdkconfig.m7.trace` is a test-only overlay enabling high-volume timing trace;
  it is not enabled in the restored product image.
- Initial dark-room testing exposed colorful low-count RGB quantization below
  the user's selected clean strip level and briefly during the STOP-response
  dip. `f776eb6` adds two LED-only guards: a 26% minimum selected output and a
  17% minimum post-effect output while lighting is enabled after startup.
- The display retains its complete slider range. The stored brightness is not
  modified by either LED guard, and a requested lighting shutdown can still
  fade to zero.

No SAS, ICD or ADR change was required: Control continues to own UI and
lighting, while Audio continues to own authoritative playback and gain.

## Software and build evidence

The inherited frame, semantic-protocol and settings-policy host suites pass
under C11 with `-Wall -Wextra -Werror -pedantic -fsanitize=undefined`.

Clean ESP-IDF v6.0 builds used commit
`662a3be354759d9487bf4b1a629fadb766cb1800` and exact SleepCube application
version `65ffc61`, which contains the M7 implementation through `f776eb6`:

| Image | Configuration | Size | SHA-256 |
| --- | --- | ---: | --- |
| Control product | ESP32-C6, 8 MB, M7, trace disabled, 1,800 s | 604,032 bytes | `52ff3c8015882a2612762835af5071f16d19790b2c18d44d0832e02d43f4bc25` |
| Control trace | ESP32-C6, 8 MB, M7 plus trace overlay | 604,704 bytes | `2d438044bccf83e80936f6ff211cfb80e80c4cafc76204e7342f2d5755e243ef` |

Both builds target 8 MB flash and completed without an observed project-code
warning. Earlier pre-correction `727285d` images were also built and used to
identify the strip defect; they are not the final product images.

## Hardware setup

- Waveshare ESP32-C6 Control Controller: `/dev/ttyACM0`, USB serial
  `AC:EB:E6:1E:67:C4`.
- AtomS3 Lite Audio Controller: `/dev/ttyACM1`, USB serial
  `44:1B:F6:F4:FE:C8`.
- UART: Waveshare GPIO16 TX to Atom GPIO2 RX; Atom GPIO1 TX to Waveshare
  GPIO17 RX; common ground; separate laptop USB power.
- Waveshare display/touch and GPIO8 external LED strip active.
- Atom image remained `d373b1a`; the 1 GB card, ATOMIC Speaker Base and speaker
  remained connected. Authoritative stored volume was 63%.
- Control trace and final product writes were hash-verified. Audio was not
  reflashed for M7.

## Physical results

### Rest gestures and adjustment view

- A short tap began audio and the visual response together. A subsequent short
  tap produced synchronized STOP feedback and a clean audio fade.
- Two deliberately moved rest-view gestures produced no playback, adjustment
  view or response pulse. Trace showed `rest_press`, `gesture_cancel` and
  `click_suppressed`, with no toggle event; one captured sequence was
  71,215,786 us, 71,695,700 us and 74,815,704 us respectively.
- A stationary long press entered adjustment mode smoothly. Trace measured
  1,020 ms. The view returned smoothly after approximately 3 s inactivity,
  preserving volume and brightness.
- Direct brightness movement changed display and strip smoothly and together;
  volume was unchanged. Direct volume behavior and authoritative confirmation
  are retained from the unchanged M5/M6 facade evidence.

### Dark-room observation and correction

The first trace image showed a clean, warm and smoothly modulated display down
to its lowest selection. At extreme low selection, however, the external strip
entered a colorful “disco” pattern, and its STOP dip briefly crossed into that
region. The user selected the lowest level that still looked clean; the trace
reported 26%.

After `f776eb6`, the corrected trace image was tested at the low setting through
ambient modulation and PLAY/STOP. The user confirmed that the strip remained a
modulated warm white and no longer entered the colorful region, including the
STOP dip. Display/strip synchronization remained good. Animation steps are
more visible at this very low strip output because of the LED's limited digital
resolution; the user accepted this characteristic.

The full display slider range remains available. During later fault testing,
trace showed stored/selected values below the strip floor, confirming that the
floor affects only physical LED output rather than the setting or display.

### Audio unavailable and recovery

- While stopped, only Atom GPIO1 TX to Waveshare GPIO17 RX was disconnected.
  Control exhausted bounded status attempts, entered neutral/unavailable Audio
  presentation and continued its ambient UI/light work.
- Brightness remained responsive. The volume control retained the last
  confirmed 63% and could not be changed. Rest presentation remained normal,
  with no playback indicator and no sound.
- Reconnecting the same line recovered through HELLO/GET_STATUS. Control
  repeatedly reported authoritative STOPPED, 63%, no fault; it did not require
  reset and did not replay PLAY. The user confirmed continued silence and
  normal rest/lighting.

### Final product restore

The exact 604,032-byte `65ffc61` product image above was restored with timing
trace disabled. It reconciled authoritative STOPPED, 63%, no fault. The user
confirmed that the speaker was silent and display/LED operation was normal.
The last trace run had stored a low brightness selection (approximately 13%);
the product restores the stored selection while the LED-only floor preserves
stable strip color.

## UX requirement disposition

| Requirement | Result | Evidence |
| --- | --- | --- |
| UX-UI-001 | PASS | Startup and adjustment timeout settle in the ambient rest view. |
| UX-UI-002 | PASS | Short taps correctly request PLAY/STOP; visible/audio responses follow authoritative progress. |
| UX-UI-003 | PASS | Stationary long press entered adjustment view; trace measured 1,020 ms. |
| UX-UI-004 | PASS | Moved gestures logged cancellation and click suppression and caused no action. |
| UX-UI-005 | PASS | User confirmed normal ambient rest presentation throughout the gate. |
| UX-UI-006 | PASS | Confirmed PLAY/STOP produced synchronized, decaying display/strip responses. |
| UX-UI-010 | PASS | Volume and brightness remain directly accessible in one view. |
| UX-UI-011 | PASS | Both spatially distinct direct controls were exercised across M5–M7. |
| UX-UI-012 | PASS | Adjustment view retained current brightness and last confirmed Audio volume. |
| UX-UI-013 | PASS | User observed return after approximately 3 s inactivity. |
| UX-UI-014 | PASS | User reported smooth entry and exit transitions. |
| UX-UI-015 | PASS | Brightness remained responsive with the Audio return path disconnected. |
| UX-UI-016 | PASS | Unavailable volume stayed at 63% and rejected adjustment. |
| UX-AUD-001 | PASS | PLAYING and STOPPED presentation tracked authoritative Audio states. |
| UX-AUD-002 | PASS | M5 authoritative-facade evidence plus M7 outage produced no speculative PLAY indication. |
| UX-AUD-003 | PASS | Link loss selected neutral rest presentation without a false pulse. |
| UX-AUD-004 | PASS | Retained M6 timer test returned to STOPPED after synchronized feedback and preserved selected brightness. |
| UX-LGT-001 | PASS | Dark-room display/strip modulation was warm and smooth. |
| UX-LGT-002 | PASS | Display and strip changed together; the corrected strip floor preserves color below its usable physical range. |
| UX-LGT-003 | PASS | No persistent bright status indicator was observed. |
| UX-LGT-004 | PASS | Responses decayed normally; corrected low-level STOP dip remained warm white. |
| UX-STA-001 | PASS | Final product boot/rest state was ambient and silent/STOPPED. |
| UX-STA-002 | PASS | Link loss did not block rest view or brightness. |
| UX-STA-003 | PASS | Reconnect recovered STOPPED/no-fault without Control reset or PLAY replay. |

## Result and limits

M7 passes its exit gate. The evidence combines this M7 physical run with M5
authoritative facade behavior and M6 timeout/persistence behavior where those
paths are unchanged; those earlier observations are not relabelled as new M7
runs.

This milestone does not close the release-wide 100 ms touch-event or 250 ms
command-confirmation limits. M7 trace markers make those measurements easier,
but the retained observations establish behavior and perceptual synchronization,
not the complete M8 latency data set. It also does not resolve playback-noise
issue `AUD-ISS-001`, which remains an explicit M8/R00 disposition item.
