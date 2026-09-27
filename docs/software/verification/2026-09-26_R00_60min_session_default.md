# R00 60-minute product-session default — verification record

**Date:** 2026-09-26; target regression completed 2026-09-27
**Status:** PASS

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
| Control product initially recorded | ESP32-C6, 8 MB, M7 product, 3,600 s | 603,216 bytes | `c21fe2262e588443589f651a9fb2d779f017bd779a363a106cacc365413343d3` |
| Audio product | ESP32-S3, 8 MB, 3,600 s local fallback | 341,952 bytes | `d62b3d311f9de8366e794b4212019447f3c16834ac6328db31096d75d6e0dd74` |

Both binaries embed exact version `3348213`. Generated Control configuration
contains `CONFIG_SC_REMOTE_PLAY_DURATION_SECONDS=3600`; the Audio binary
contains the `60-minute local default` identity. No endurance overlay is enabled
in either product build.

## Final flashed candidates

Before target testing, both USB identities were resolved rather than inferred
from device order: Waveshare Control `AC:EB:E6:1E:67:C4` enumerated as
`/dev/ttyACM0`, and Atom Audio `44:1B:F6:F4:FE:C8` enumerated as
`/dev/ttyACM1`.

The first Control `idf.py flash` invocation reconfigured the preserved build
directory under the later documentation-only repository checkpoint `8fdcb2b`,
changing the embedded application version and binary hash. This was detected
from the build output before target evidence was collected. A fresh isolated
build was therefore made from detached implementation commit `3348213`, and
the resulting files were flashed directly without another build step. No
verification result is credited to the superseded interim image.

| Final flashed image | Embedded version | Size | SHA-256 |
| --- | --- | ---: | --- |
| Control product | `3348213` | 603,216 bytes | `c526231c244d70e2036069c90cd709019b1472d947b4c5e5918dcb20958e0036` |
| Audio product | `3348213` | 341,952 bytes | `d62b3d311f9de8366e794b4212019447f3c16834ac6328db31096d75d6e0dd74` |

All bootloader, partition-table and application writes completed with esptool
hash verification. Control configuration was rechecked as
`CONFIG_SC_REMOTE_PLAY_DURATION_SECONDS=3600`; the preserved Audio application
was rechecked for both embedded version `3348213` and the `60-minute local
default` identity.

## Target regression

The exact final candidates passed the targeted on-device regression on
2026-09-27:

- synchronized startup reached repeated authoritative `STOPPED`,
  `remaining=0s`, `fault=0`, with persisted volume 59%;
- the user confirmed silent startup, normal display/LED-strip rest presentation,
  and no click, pop or false playback indication;
- Control logged `remote PLAY(3600s) requested`, Audio accepted the correlated
  command and logged `session start duration=3600s`;
- both sides reported `STARTING` and then `PLAYING` with `remaining=3600s`,
  followed by the expected countdown and `fault=0`;
- the manual STOP was accepted, Audio logged the configured one-second fade,
  then reached `STOPPED`, manual reason 1, `remaining=0s`, `fault=0`;
- repeated post-stop status polls remained `STOPPED` without automatic restart;
- the user reported a good response, clean fade and fully silent speaker.

## Disposition

`R00-DUR-001` passes. The targeted candidate result identifies the exact normal
product artifacts and confirms their configured 3,600-second PLAY/STOP path.
M8-END-001 supplies the retained complete 3,600-second runtime/deadline evidence,
so another hour-long run is not required. M9 release preparation may begin.
