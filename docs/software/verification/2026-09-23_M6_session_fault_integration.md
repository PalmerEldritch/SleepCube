# M6 session and fault integration — development record

**Date:** 2026-09-23
**Status:** PASS — implementation, clean builds, target fault injection,
finite-session behavior and power-cycle persistence gates complete.

## Scope

M6 closes the remaining system-level session, UART-loss and persisted-setting
requirements without changing Audio ownership, the semantic wire format or the
M5 product interaction model. Existing M2–M5 evidence is retained where the
mechanism is unchanged.

## Implementation

- `709512c` adds one-second Control-side GET_STATUS health probes. Application
  requests take priority before a new probe; a probe uses the existing 250 ms,
  three-attempt bound.
- Exhausting the probe marks audio unsynchronized/unavailable, leaves UI and
  lighting tasks running, and retries HELLO followed by GET_STATUS. No PLAY is
  reconstructed or replayed.
- The unavailable presentation is neutral: confirmed-playing boost is removed,
  unconfirmed volume input is ignored, and link loss alone does not create a
  start/stop pulse or modify selected brightness.
- Normal product PLAY remains finite at 1,800 seconds through
  `sdkconfig.m6.defaults`. Commit `ab214fe` adds the explicit test-only
  `sdkconfig.m6.timer-test` overlay selecting 30 seconds. Commit `9e28ab7`
  makes the serial-console log distinguish that fixed diagnostic command from
  the configured product UI duration.
- Control brightness and Audio volume validate out-of-range persisted values,
  clamp them to supported bounds and attempt to repair the stored value.

## Software evidence

Strict host commands used `-std=c11 -Wall -Wextra -Werror -pedantic
-fsanitize=undefined`. Frame, semantic protocol and new Control setting-policy
tests all pass.

Clean ESP-IDF v6.0 builds pass. The final Control images were rebuilt from
committed `9e28ab7`; the unchanged Audio image deployed for the gate is
`d373b1a`:

| Image | Target/configuration | Size | SHA-256 |
| --- | --- | ---: | --- |
| Control product | ESP32-C6, 8 MB, M6, 1,800 s | 603,680 bytes | `a601dab0f8c9308e790419bf83c3afa97c361b9d2a4456ff5c65954c785adda1` |
| Control timer test | ESP32-C6, 8 MB, M6 + 30 s overlay | 603,680 bytes | `701ff233c8d1bc63cf439f054921791a2fc53cbe20f9ab9db00890b18118aa75` |
| Audio | ESP32-S3, 8 MB | 341,680 bytes | `917469ceab5145273ebaac7577bc70d3bedaf01cd2c02c91483ded2f22a18411` |

The retained local Control backend also builds after the persistence-policy
change. No project-code warning was observed.

## Reused evidence

- M2 proves a normal 1,800-second independent Audio deadline, timeout fade,
  persistent volume restoration, invalid-volume clamping and power-interruption
  safe startup.
- M4 proves malformed/truncated/unsupported traffic rejection, duplicate and
  out-of-range command handling, both boot orders, both independent reset
  directions and Audio deadline survival across Control reset.
- M5 proves product-facade touch/volume behavior, neutral operation while Audio
  is unavailable, brightness independence and silent Audio restart/recovery.

These results are not re-labelled as new M6 runs; they cover unchanged lower
layers and reduce the physical M6 gate to the integration behavior below.

## Hardware setup and image identity

- Waveshare ESP32-C6 Control Controller: `/dev/ttyACM0`, USB serial
  `AC:EB:E6:1E:67:C4`.
- AtomS3 Lite Audio Controller: `/dev/ttyACM1`, USB serial
  `44:1B:F6:F4:FE:C8`.
- Full-duplex UART remained GPIO16→GPIO2 and GPIO1→GPIO17 with common ground;
  both controllers used separate laptop USB power.
- Audio used the mounted 1 GB card, canonical `/sleepcube/audio.wav`, ATOMIC
  Speaker Base and connected speaker. The GPIO8 LED strip and Waveshare display
  were active.
- The physical timer/fault gate ran with timer-test Control app `d373b1a`
  (603,632 bytes) and Audio app `d373b1a` (341,680 bytes). Flash writes were
  hash-verified. The timer-test file SHA-256 was not retained before its build
  directory was rebuilt; target boot identified Control ELF prefix `777c30016`
  and Audio ELF prefix `3445b5b93`.
- The final deployed Control product image is the exact `9e28ab7` image in the
  table above. Its post-flash boot identified ELF prefix `7b53b5ca5`, 8 MB
  flash and `product UI=1800s`.

## Physical results

### Finite timer and presentation

- A rest-view tap issued `PLAY(30s)`, which was accepted and progressed through
  `STARTING` to `PLAYING`.
- The authoritative remaining time counted down, then entered `STOPPING` with
  reason `TIMER_EXPIRED` and a five-second fade. It reached `STOPPED` about
  30.1 seconds after the PLAY request; no fault was reported.
- The user confirmed a smooth fade, silent stopped output, and stopped visual
  feedback beginning with the fade. The normal transient stop dip occurred and
  the display/strip returned to their unchanged selected brightness afterward.

### UART signal loss and recovery

- During active playback, only Atom GPIO1 TX → Waveshare GPIO17 RX was removed;
  USB power, common ground and the opposite UART direction remained connected.
- The next one-second status probe exhausted three 250 ms attempts, logged an
  actionable timeout, marked authoritative audio unavailable and selected the
  neutral presentation. HELLO recovery attempts remained bounded.
- Brightness remained responsive throughout the outage. Audio continued locally
  to the original deadline, faded and stopped cleanly without Control status.
- After the signal was restored, HELLO/INFO followed by GET_STATUS reconciled
  `STOPPED`, `TIMER_EXPIRED`. No PLAY was reconstructed or replayed; the user
  confirmed continued silence and normal lighting.

### Control reset during playback

- A separate UI PLAY reached `PLAYING`; only the Waveshare reset button was
  pressed while its authoritative status still showed time remaining.
- Audio continued without restart or level change and stopped on the original
  deadline. The new Control boot/session synchronized through HELLO/GET_STATUS
  to the existing Audio session and `STOPPED`, without issuing PLAY.
- The user confirmed continuous-until-deadline behavior and silence afterward.

### Persistence and final product restore

- The UI stored brightness 42%; SET_VOLUME was accepted and stored at 63%.
- The 1,800-second product image was restored without erasing NVS. Both USB
  supplies were removed, five seconds elapsed, and both controllers were
  reconnected without a user playback request.
- Control restored brightness 42%. Audio restored volume 63%, mounted and
  validated content, enabled I2S in its silent state and reported `STOPPED` with
  its 1,800-second local default. The user confirmed silent startup and normal
  display/LED operation.
- Final Control `9e28ab7` was then flashed with hash verification and again
  reconciled `STOPPED`, volume 63%, no fault, while retaining brightness 42%.

## Result and limits

M6 passes its SRS-SES and SRS-FLT integration gate. Evidence combines the M6
physical runs above with unchanged M2 finite-duration/persistence evidence and
M4 malformed-message/semantic protocol evidence. The 30-second timer overlay
accelerates the same configured finite-session path; the deployed product
configuration and Audio local default are both 1,800 seconds.

This milestone does not claim exhaustive content-fault permutations, release
latency measurements, final dark-room UX tuning, or resolution of playback-noise
issue `AUD-ISS-001`. That issue remains open for an explicit R00 disposition.
