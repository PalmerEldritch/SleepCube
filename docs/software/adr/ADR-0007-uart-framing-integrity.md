# ADR-0007 — Bounded COBS/CRC UART framing

**Status:** Accepted
**Date:** 2026-09-21
**Supersedes:** None
**Superseded by:** None

## Context

ADR-0004 selected a 3.3 V full-duplex UART semantic link but deferred the wire
representation. M3 needs unambiguous boundaries, resynchronization after line
noise or reset, integrity checking, bounded memory, and command/response
correlation without exposing audio samples or implementation details.

## Decision

Use a compact binary packet encoded with COBS and terminated by a zero byte.
The decoded packet is, in order: protocol version (1 byte), message type
(1 byte), sequence number (little-endian 16-bit), payload length (1 byte),
payload (0–32 bytes), and CRC-16/CCITT-FALSE (little-endian 16-bit) over all
preceding decoded bytes. Version 1 is the initial version. Encoded packets are
bounded to 41 bytes including the terminating zero.

Receivers shall discard empty delimiters, malformed COBS, length/version/CRC
failures, and overlong frames without invoking semantic handlers. An overlong
or time-expired partial frame shall be discarded through the next delimiter
before parsing resumes. M3 uses a 100 ms inter-byte expiry; the value may be
tuned with evidence without changing the packet format.

Each sender assigns a 16-bit sequence to a new request. Responses echo the
request sequence. The sequence correlates messages but does not, by itself,
guarantee exactly-once execution. M4 shall add per-session duplicate/retry
handling for state-changing commands, especially PLAY, and reconcile against
authoritative Audio Controller status. A reset starts a new link session and
requires status synchronization; an old sequence cannot justify resuming audio.

UART is nominally 115200 8N1. The driver uses a dedicated UART peripheral
mapped to the selected pins; the USB Serial/JTAG console remains independent.
M3 reserves type `0x70` for diagnostic PING and `0x71` for diagnostic PONG.
These are transport-only and must never invoke playback operations.

## Consequences

### Positive

- zero delimiter enables deterministic byte-stream resynchronization;
- CRC detects common corruption; length and payload bounds limit memory use;
- a shared codec can be host-tested and used by both controllers;
- request/response correlation is available for M4.

### Costs / Risks

- COBS and CRC add a small implementation and test surface;
- 16-bit sequences wrap and require bounded outstanding-request handling;
- CRC is an integrity check, not authentication or a guarantee of delivery;
- M4 still needs explicit idempotency, timeouts and state reconciliation.

## Alternatives Considered

- Newline-delimited text: easy to inspect but needs escaping, strict length
  limits, and more ambiguous partial/invalid-line recovery.
- Raw sync word plus length: smaller encoder, but arbitrary payload can contain
  the sync word and corrupted lengths complicate resynchronization.
- SLIP plus CRC: viable, but COBS gives a simple worst-case size bound for these
  short fixed-budget packets.

## Verification / Implications

- Host tests shall cover round trip, maximum payload, invalid version/length,
  bad CRC/COBS, overlong data, truncated-frame expiry and subsequent resync.
- M3 target tests shall show PING/PONG in both directions and safe behavior when
  malformed/truncated bytes are injected.
- M4 shall define semantic type IDs, payloads, duplicate handling, link-session
  identity, command timeout and authoritative status reconciliation before
  accepting remote PLAY.

## References

- `docs/software/05_ICD.md` sections 2–3
- `docs/software/adr/ADR-0004-uart-semantic-audio-link.md`
