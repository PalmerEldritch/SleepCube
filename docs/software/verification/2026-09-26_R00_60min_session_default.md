# R00 60-minute product-session default — verification record

**Date:** 2026-09-26
**Status:** IN PROGRESS — implementation, host regressions and exact clean builds
pass; target regression remains.

## Scope

The user approved one post-M8 R00 behavior adjustment: change the normal
playback-session default from 1,800 to 3,600 seconds. The session remains finite
and the existing five-second timeout fade must still reach silence at the
deadline. The semantic protocol's accepted range remains 1 through 3,600
seconds.

This change does not alter controller ownership, UART framing, idempotency,
retry behavior, content format, gain/fades or persisted settings. Control still
selects the product duration and Audio still establishes and independently
enforces the requested deadline.

## Implementation

- Control Kconfig and the current M7 product defaults now select 3,600 seconds;
  the historical M5/M6 profiles retain the values used for their evidence.
- The Audio local-button fallback now selects the same finite 3,600-second
  duration; the separate M2 endurance identity remains available but no longer
  changes the duration.
- The SRS, ICD, current operating instructions and ADR-0008 amendment note now
  state the 60-minute R00 product value.
- Historical verification records retain their original 1,800-second values
  because they describe the images actually tested at those milestones.

## Evidence carried forward

M8-END-001 already exercised a complete 3,600-second PLAY through the normal
distributed facade and Audio deadline path. It reached timer-expired `STOPPED`,
stop reason 2 and `fault=0`, with no user-observed audible or visual artifact.
That evidence validates the selected duration and runtime path, but it does not
identify the new exact product artifacts.

## Automated verification

Strict C11/UBSan frame, semantic-protocol and settings-policy tests pass. All
three timing-analyzer unit tests also pass.

Fresh out-of-tree ESP-IDF v6.0 pre-commit builds pass for the current Control
product profile and Audio product profile. Exact clean builds from committed
implementation `3348213` then pass with these identities:

| Image | Target/configuration | Size | SHA-256 |
| --- | --- | ---: | --- |
| Control product | ESP32-C6, 8 MB, M7 product, 3,600 s | 603,216 bytes | `c21fe2262e588443589f651a9fb2d779f017bd779a363a106cacc365413343d3` |
| Audio product | ESP32-S3, 8 MB, 3,600 s local fallback | 341,952 bytes | `d62b3d311f9de8366e794b4212019447f3c16834ac6328db31096d75d6e0dd74` |

Both binaries embed exact version `3348213`. Generated Control configuration
contains `CONFIG_SC_REMOTE_PLAY_DURATION_SECONDS=3600`; the Audio binary
contains the `60-minute local default` identity. No endurance overlay is enabled
in either product build.

## Remaining verification

1. Flash the exact candidates, confirm silent synchronized startup, start one
   session and verify both sides report a 3,600-second finite duration.
2. Stop the session normally and verify a clean fade to silent `STOPPED` with no
   fault or automatic restart.

The full 60-minute endurance run need not be repeated unless the targeted
regression differs from the M8-END-001 path or exposes a fault; the only product
change selects the already exercised 3,600-second value.
