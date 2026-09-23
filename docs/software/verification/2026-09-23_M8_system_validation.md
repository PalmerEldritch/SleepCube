# M8 system validation — verification record

**Date:** 2026-09-23
**Status:** IN PROGRESS — requirements/ICD audit and validation tooling prepared;
final target evidence is pending.

## Scope

M8 produces the complete R00 evidence set from the distributed product
baseline. Existing M1–M7 records are retained where the implementation path is
unchanged; M8 does not repeat a test merely to rename earlier evidence. It does
repeat release-sensitive timing, integrated endurance, content-fault and final
candidate recovery/persistence checks.

## Audit result

The audit found 55 active SRS IDs and 29 active ICD IDs. Five active SRS rows
were missing from the core VVM table despite existing implementation evidence:
SRS-SYS-004, SRS-STA-002, SRS-UI-006, SRS-LGT-003 and SRS-LGT-005. They are now
included. The VVM now maps every active SRS, UX and ICD ID.

The SRS draft's old “deferred” list was stale: ADR-0007/ADR-0008 and the frozen
ICD already resolve framing, integrity, acknowledgement and retry behavior.
M1–M7 target listening also established acceptable 1 s start/manual-stop,
250 ms volume and 5 s timeout transitions; those values are now the explicit
R00 acceptance baseline. `AUD-ISS-001` remains a release issue, not an
unspecified requirement.

### SRS execution audit

`PASS` means retained evidence is sufficient for the stated requirement.
`PARTIAL` or `PENDING` identifies exact M8 work. `RETEST` means earlier evidence
passes but the M8 release-candidate regression is still required.

| Requirement | Audit | Evidence or exact remaining work |
| --- | --- | --- |
| SRS-SYS-001 | PASS | Separate Control/Audio projects and physical UART integration, M3–M4. |
| SRS-SYS-002 | PASS | Control-owned product/UI state and authoritative remote facade, SAS/M5. |
| SRS-SYS-003 | PASS | Storage, WAV, buffering, gain and I2S remain Audio-local, M2/M4 inspection. |
| SRS-SYS-004 | PASS | Display/touch/GPIO8 lighting exist only on Control and pass M7 target checks. |
| SRS-SYS-005 | PASS | M6/M7 link loss leaves UI and lighting usable. |
| SRS-STA-001 | PASS | M5–M7 neutral/unavailable startup and authoritative reconciliation. |
| SRS-STA-002 | PASS | Repeated normal boot/power-cycle observations remain silent with no PLAY. |
| SRS-STA-003 | PASS | M4 verifies both boot orders and independent resets. |
| SRS-STA-004 | PASS | Audio reset changes session and Control reacquires state without restart. |
| SRS-STA-005 | PASS | Audio reset during playback returns silent/STOPPED. |
| SRS-UI-001 | PASS | M7 ambient rest view is the steady state. |
| SRS-UI-002 | PASS | M5/M7 rest taps toggle finite remote playback. |
| SRS-UI-003 | PASS | M7 direct volume/brightness view passes. |
| SRS-UI-004 | PASS | M7 approximately three-second inactivity return passes. |
| SRS-UI-005 | PASS | M7 brightness remains usable with Audio return path absent. |
| SRS-UI-006 | PASS | Normal interaction uses rest plus one direct adjustment view, no nested menu. |
| SRS-LGT-001 | PASS | M7 dark-room external warm ambient rendering passes. |
| SRS-LGT-002 | PASS | M7 smooth brightness behavior and low-output correction pass. |
| SRS-LGT-003 | PASS | M5/M7 display and strip respond together. |
| SRS-LGT-004 | PASS | M5–M7 stop, timeout, reset and link loss retain lighting. |
| SRS-LGT-005 | PASS | Semantic Audio state drives transient light response; no samples cross UART. |
| SRS-AUD-001 | PASS | Remote facade exposes semantic control only. |
| SRS-AUD-002 | PASS | PLAY, STOP and SET_VOLUME pass on target, M4–M7. |
| SRS-AUD-003 | PASS | Unavailable, transitional, stopped, playing and fault states are exposed. |
| SRS-AUD-004 | PASS | One-second start/stop transitions are implemented and audibly clean. |
| SRS-AUD-005 | PASS | 250 ms volume changes are implemented and artifact-free in target tests. |
| SRS-AUD-006 | PASS | M2 content loops continuously without user track selection. |
| SRS-AUD-007 | PASS | M6 Control reset/link loss leaves Audio buffers and deadline autonomous. |
| SRS-AUD-008 | PASS | Canonical 44.1 kHz/16-bit mono PCM path validates and plays, M2 onward. |
| SRS-AUD-009 | PARTIAL | Missing card and unsupported WAV pass. Add missing-file and malformed/truncated-file target cases on the hardened reader. |
| SRS-SES-001 | PASS | Wire/server bounds and normal facade make unbounded PLAY impossible. |
| SRS-SES-002 | PASS | Product profile requests 1,800 seconds. |
| SRS-SES-003 | PASS | M2/M6 prove Audio-owned deadline through Control disruption. |
| SRS-SES-004 | PASS | Control presents state but Audio alone enforces deadline. |
| SRS-SES-005 | PASS | M2/M4/M6 prove five-second timeout fade, STOPPED and event. |
| SRS-SES-006 | PASS | M6 timeout preserved selected brightness after transient response. |
| SRS-COM-001 | PASS | Dedicated mapped 3.3 V full-duplex UART works bidirectionally. |
| SRS-COM-002 | PASS | Strict frame tests and M3 target probes reject bad/truncated frames. |
| SRS-COM-003 | PASS | HELLO/INFO/GET_STATUS synchronization passes both boot orders. |
| SRS-COM-004 | PASS | Version/type/payload rejection passes strict host and target tests. |
| SRS-COM-005 | PASS | M6 link loss did not defeat independent stop. |
| SRS-COM-006 | PASS | ICD/type/API inspection contains no sample-data path. |
| SRS-SET-001 | PASS | M6 normal power cycle restored Control brightness. |
| SRS-SET-002 | PASS | M2/M6 normal power cycles restored Audio volume. |
| SRS-SET-003 | PASS | Strict policy tests plus Audio target fallback validate/clamp stored values. |
| SRS-SET-004 | PASS | Audio never restores playback after reset/power loss. |
| SRS-FLT-001 | PASS | M5–M7 Audio faults/loss leave UI and light tasks responsive. |
| SRS-FLT-002 | PASS | M4/M6 Control reset leaves existing Audio deadline unchanged. |
| SRS-FLT-003 | PASS | M5–M7 recovery reacquires authoritative status before presentation. |
| SRS-FLT-004 | PASS | Strict semantic tests reject malformed, partial and out-of-range input safely. |
| SRS-FLT-005 | PASS | M3/M6 parse, retry, timeout and recovery logs are actionable. |
| SRS-SYS-010 | PENDING | Capture at least 20 nominal touch emit→app acceptance samples; maximum shall be ≤100 ms. |
| SRS-COM-010 | PENDING | Capture at least 20 nominal user Audio event→correlated acknowledgement samples; maximum shall be ≤250 ms. |
| SRS-LGT-010 | PASS | M5–M7 report continuous display/strip animation during UI/UART work. |
| SRS-AUD-010 | RETEST | M2 60-minute run passed the same audio pipeline; repeat through the distributed facade with the M8 3,600-second overlay. |

### ICD execution audit

All 29 ICD IDs have implementation plus host/target evidence, now enumerated in
`06_VVM.md`. The remaining M8 physical setup check is to visually confirm that
the Grove red 5 V contact remains disconnected and insulated; bidirectional
traffic already proves TX/RX/common-ground continuity. Timing retest does not
change the frozen semantic contract.

## M8 validation tooling and implementation

- Control transport logs the exact first-transmission timestamp, correlated
  response timestamp and request/response latency for each request. Retries keep
  the original timestamp so a non-nominal retry cannot appear artificially fast.
- `firmware/control/tools/analyze_m8_timing.py` pairs structured UI traces with
  application receipt and correlated PLAY/STOP/SET_VOLUME responses. It reports
  minimum, median, 95th percentile and maximum and fails either SRS threshold or
  insufficient sample count. Standard-library unit tests cover toggles, ANSI
  monitor output and coalesced volume traffic.
- `sdkconfig.m8.endurance` changes only the normal facade's finite request from
  1,800 to 3,600 seconds for the integrated endurance run.
- The Audio WAV reader now rejects a data chunk whose declared end exceeds the
  actual file size, before enabling playback, with an actionable truncated-WAV
  diagnostic.
- `firmware/control/README.md` records clean product/trace/endurance build,
  flash, monitoring and timing-analysis procedures.

Strict C11/UBSan frame, semantic-protocol and settings-policy regressions pass.
The timing-analyzer unit suite passes. ESP-IDF v6.0 clean builds pass for the
ESP32-C6/8 MB product, trace and endurance profiles and the ESP32-S3/8 MB Audio
profile. They were rebuilt after checkpoint commit `e4d31cc` so every image
embeds that exact application version:

| Image | Size | SHA-256 |
| --- | ---: | --- |
| Control product | 604,288 bytes | `518e26225fef7253c1b28959ab7fd09651345a6dca4f9f36b964fc593b77b384` |
| Control M8 timing trace | 604,976 bytes | `735b4bb778201fbf71da6810d909d553640abfa5923fd889c70579b54ba726d5` |
| Control M8 3,600-second endurance | 604,288 bytes | `ee3d3930dde79ba657954454c5339ae4cd6e9faf45387027a73b84dc04cc8cf9` |
| Audio product | 341,952 bytes | `fc95fcdfca352c096ae796fa1ac56c01a2b3fce4a3b2099b4560c0a834e0abb7` |

No project-code warning was observed. Component-manager registry checks were
skipped because the build host had no registry connection; all dependencies
were already resolved locally and each build completed successfully.

## Remaining physical sequence

1. Connect both controllers in the established setup; visually confirm common
   ground and that no 5 V rail crosses between boards.
2. Flash the exact committed Audio image and M8 Control trace image. Capture at
   least 20 normal touch acceptances and 20 acknowledged Audio commands, analyze
   the retained log, and observe continuous lighting.
3. Perform five independent Control resets and five independent Audio resets,
   including active-playback cases, checking silence/safe recovery and no replay.
4. Mount the card on the laptop. Preserve and hash the canonical asset, audition
   that exact mono WAV through a known-clean output for `AUD-ISS-001`, then run
   missing-file and malformed/truncated-WAV target boots. Restore and re-hash the
   canonical file.
5. Flash the M8 3,600-second Control overlay and run one integrated 60-minute
   session, retaining both controller logs and checking for underrun/fault.
6. Restore the exact product image, perform three normal dual-controller power
   cycles to verify persistence and silent startup, and record the final
   release-candidate image identities.

## Open release items

- quantitative timing capture;
- final-candidate repeated reset/power-cycle checks;
- missing-file and malformed/truncated content fault injection;
- distributed 60-minute endurance regression;
- evidence-backed `AUD-ISS-001` disposition;
- exact final release-candidate build/hash and product restore.
