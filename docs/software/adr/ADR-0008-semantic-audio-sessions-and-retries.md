# ADR-0008 — Semantic audio sessions, idempotency and retry recovery

**Status:** Accepted  
**Date:** 2026-09-23  
**Supersedes:** None  
**Superseded by:** None

## Context

ADR-0007 provides bounded framing, integrity checks and request/response
sequence correlation, but a sequence alone cannot prevent a lost PLAY response
from causing a retry to extend or duplicate a session. Either controller may
reset independently. Control must distinguish a new Audio instance from a slow
response, while a rebooted Control must reacquire currently playing Audio state
without implicitly stopping, restarting or extending it.

## Decision

Each controller generates a non-zero 32-bit boot/session ID. `HELLO` carries the
Control session and `INFO` echoes it with the current Audio session. Every
state-changing command carries both session IDs plus a non-zero monotonically
increasing 32-bit command ID.

Audio retains the latest command type, complete payload, ID and result for the
current Control session. An exact repeat of that command returns `DUPLICATE`
without invoking the playback owner. Reuse of that ID for different content is
rejected, as are older IDs. A new Control session clears this duplicate cache
but does not change Audio playback or its deadline. A new Audio session makes
old commands invalid.

Control permits one outstanding request. It waits 250 ms and transmits the exact
same framed request at most three times. If all attempts fail, state becomes
unsynchronized; Control performs a fresh `HELLO` and `GET_STATUS` but does not
automatically reissue the uncertain state-changing command. Responses echo the
16-bit transport sequence; unsolicited events use sequence zero.

`RESULT/ACCEPTED` means the semantic operation was validated and queued. The
subsequent Audio status/event is authoritative for asynchronous playback and
volume state. Normal product PLAY is 1,800 seconds. The protocol accepts only
finite durations from 1 to 3,600 seconds so short integration tests and the
bounded endurance configuration remain possible without an unbounded path.

## Consequences

### Positive

- a lost PLAY acknowledgement can be retried without extending the deadline;
- independent controller reset is distinguishable from delayed traffic;
- rebooted Control can reconcile an already-running finite Audio session;
- retries remain bounded and cannot stall UI or lighting indefinitely;
- command and status payloads remain below ADR-0007's 32-byte limit.

### Costs / Risks

- the Audio endpoint retains per-session duplicate state;
- Control must explicitly represent unsynchronized remote state;
- version 1 serializes requests and does not support multiple in-flight
  operations;
- `ACCEPTED` is not final completion, so consumers must observe authoritative
  status/events;
- command-ID wrap requires establishment of a new Control session.

## Alternatives Considered

- Use only the 16-bit frame sequence — rejected because reset/wrap and a retry
  after lost response do not provide durable operation identity.
- Make all commands desired-state operations without IDs — rejected because a
  repeated PLAY duration could still unintentionally replace or extend a live
  deadline.
- Persist command IDs across power loss — rejected because boot-session IDs
  already invalidate stale traffic and playback is intentionally not restored.
- Automatically replay uncertain commands after reconnection — rejected because
  reconciliation must not create audio output from stale user intent.

## Verification / Implications

- host tests cover session mismatch, duplicate PLAY suppression, command-ID
  reuse, stale commands, finite bounds, reset reconciliation and status events;
- two-controller tests shall verify both boot orders, independent resets,
  timeout recovery, duplicate replay and no automatic PLAY;
- M5 shall consume the link client behind the Control audio-service facade
  without moving retry waits into UI or lighting tasks.

## References

- `docs/software/05_ICD.md`
- `docs/software/04_SAS.md`
- `docs/software/adr/ADR-0004-uart-semantic-audio-link.md`
- `docs/software/adr/ADR-0007-uart-framing-integrity.md`
- `firmware/shared/protocol/sc_audio_protocol.c`
- `firmware/shared/transport/sc_audio_link.c`
