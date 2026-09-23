# M6 session and fault integration — development record

**Date:** 2026-09-23
**Status:** IN PROGRESS — implementation and clean builds pass; target flash and
physical fault/session/persistence gates pending.

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
  `sdkconfig.m6.timer-test` overlay selecting 30 seconds.
- Control brightness and Audio volume validate out-of-range persisted values,
  clamp them to supported bounds and attempt to repair the stored value.

## Software evidence

Strict host commands used `-std=c11 -Wall -Wextra -Werror -pedantic
-fsanitize=undefined`. Frame, semantic protocol and new Control setting-policy
tests all pass.

Clean ESP-IDF v6.0 builds from committed `ab214fe` pass:

| Image | Target/configuration | Size | SHA-256 |
| --- | --- | ---: | --- |
| Control product | ESP32-C6, 8 MB, M6, 1,800 s | 603,648 bytes | `f141b50dfe046837111a53313503aa6441e36d83c0c82175431d95975b5b6606` |
| Control timer test | ESP32-C6, 8 MB, M6 + 30 s overlay | 603,632 bytes | `d0559299d3ab1603891fb3f4e0630584b56a460d64ee35322a62b24940ec188d` |
| Audio | ESP32-S3, 8 MB | 341,680 bytes | `a1147223238380bfe8f9bd8603aecc1c0ef02fdbb023c5fbbf80d1a034949e1d` |

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

## Pending hardware gate

1. Flash exact `ab214fe` Audio and verify silent STOPPED startup with persisted
   volume.
2. Flash the 30-second Control timer-test image; start through the product UI
   and confirm TIMER_EXPIRED fade, stopped visual feedback and unchanged
   brightness.
3. During an active 30-second session, disconnect only one UART signal (not
   USB, 5 V or common ground). Confirm bounded degraded detection, responsive
   brightness, independent Audio timeout, and HELLO/GET_STATUS recovery without
   playback replay after reconnect.
4. Reset Control during active playback and confirm Audio continues only to the
   existing deadline; after Control recovery, authoritative state is reconciled
   without extending or replaying the session.
5. Set non-default volume and brightness, restore the 1,800-second product
   Control image, power-cycle both controllers normally, and confirm both values
   return while playback remains stopped.

M6 passes only after the SRS-SES and SRS-FLT integration rows have sufficient
physical evidence. `AUD-ISS-001` remains outside this milestone and open for
R00 disposition.
