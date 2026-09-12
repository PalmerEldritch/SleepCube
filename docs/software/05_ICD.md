# SleepCube Inter-Controller Interface Control Document

**Revision:** R00 draft

## 1. Purpose

This document defines the logical and electrical contract between the Control Controller and Audio Controller.

The initial revision freezes the semantic interface and failure rules while intentionally leaving the exact wire framing/encoding open until the transport implementation milestone.

## 2. Physical interface

| ID | Requirement |
| --- | --- |
| ICD-PHY-001 | The interface shall use 3.3 V logic-level full-duplex UART and a common ground. |
| ICD-PHY-002 | The nominal UART configuration shall be 115200 baud, 8 data bits, no parity, 1 stop bit (115200 8N1). |
| ICD-PHY-003 | R00 shall use dedicated GPIO not shared with real-time audio output, display/touch or external RGB-light timing. |
| ICD-PHY-004 | Final TX/RX GPIO assignments shall be documented before M3 integration testing. |

## 3. Protocol principles

| ID | Requirement |
| --- | --- |
| ICD-PRO-001 | The protocol shall exchange semantic commands, responses and events; it shall not transport audio samples. |
| ICD-PRO-002 | Each message shall have an unambiguous boundary and message type. |
| ICD-PRO-003 | Each message shall carry a protocol version or be interpreted within an explicitly negotiated protocol version. |
| ICD-PRO-004 | The receiver shall be able to reject malformed, incomplete or unsupported messages without performing an unsafe action. |
| ICD-PRO-005 | Commands that change audio state shall support correlation with an acknowledgement/result or resulting authoritative status. |
| ICD-PRO-006 | The exact representation (binary framed protocol vs compact textual protocol), integrity field and sequence-number strategy are TBD and shall be frozen by ADR before M4 completion. |

## 4. Required command semantics

The following logical commands are mandatory. Names below are semantic names, not yet wire encodings.

| Command | Direction | Required parameters | Meaning |
| --- | --- | --- | --- |
| `HELLO/GET_INFO` | Control → Audio | protocol version/capabilities as required | Establish compatibility and retrieve implementation identity/capability information |
| `GET_STATUS` | Control → Audio | none | Request authoritative current audio state |
| `PLAY` | Control → Audio | finite session duration; optionally command ID | Begin playback using configured/preloaded content |
| `STOP` | Control → Audio | optionally command ID | Request controlled manual fade/stop |
| `SET_VOLUME` | Control → Audio | supported volume value | Set playback volume |

No command shall expose remote decoder setup, I2S clocking or remote filesystem manipulation to normal Control Controller application code.

## 5. Required status/event semantics

| Status/Event | Direction | Meaning |
| --- | --- | --- |
| `READY` | Audio → Control | Audio services initialized and command processing available |
| `STATUS` | Audio → Control | Authoritative current state and relevant values |
| `PLAYING` | Audio → Control | Playback has successfully entered active state |
| `STOPPED` | Audio → Control | Playback is inactive |
| `VOLUME` | Audio → Control | Confirmed current volume |
| `TIMER_EXPIRED` | Audio → Control | Session deadline caused playback termination/fade |
| `ERROR` | Audio → Control | Audio operation or subsystem fault with machine-readable error code |

Implementations may combine `PLAYING`, `STOPPED` and `VOLUME` into a structured `STATUS` message provided equivalent semantics are preserved.

## 6. Audio state model

Minimum externally observable states:

```text
NOT_READY / UNKNOWN
        ↓
      STOPPED
        ↓ PLAY
      PLAYING
        ↓ STOP / timeout / fault
      STOPPED
```

Optional transient `STARTING`, `STOPPING` or `FAULT` states may be exposed if useful, but Control firmware shall not depend on them unless added normatively to this ICD.

## 7. PLAY contract

| ID | Requirement |
| --- | --- |
| ICD-AUD-001 | `PLAY` shall contain or imply a finite non-zero session duration accepted by the Audio Controller. |
| ICD-AUD-002 | A `PLAY` request with absent, zero, out-of-range or otherwise invalid duration shall be rejected. |
| ICD-AUD-003 | Successful `PLAY` shall establish the Audio Controller's independent timeout before or atomically with entry into playing state. |
| ICD-AUD-004 | Receipt of duplicate/retried `PLAY` messages shall not accidentally create an unbounded or multiplicatively extended session. Exact idempotency strategy is TBD with framing design. |

## 8. STOP contract

| ID | Requirement |
| --- | --- |
| ICD-AUD-010 | `STOP` shall request a controlled fade followed by stopped state. |
| ICD-AUD-011 | `STOP` received while already stopped shall be safe and may be acknowledged as already satisfied. |

## 9. Volume contract

| ID | Requirement |
| --- | --- |
| ICD-AUD-020 | Volume shall use a bounded logical range independent of amplifier implementation. R00 application-facing range is 0-100 percent. |
| ICD-AUD-021 | Out-of-range values shall be rejected or safely clamped according to the final protocol definition; they shall never wrap numerically. |
| ICD-AUD-022 | The Audio Controller shall report the resulting confirmed volume after successful application or via subsequent status. |

## 10. Startup and resynchronization

| ID | Requirement |
| --- | --- |
| ICD-SYN-001 | The protocol shall not assume either controller boots first. |
| ICD-SYN-002 | After establishing communication, the Control Controller shall obtain authoritative Audio Controller status before treating audio state as synchronized. |
| ICD-SYN-003 | An Audio Controller reset shall result in safe non-playing startup state. |
| ICD-SYN-004 | A Control Controller reset during playback shall not remove or extend the Audio Controller's existing finite session deadline. |
| ICD-SYN-005 | Reconnection shall not automatically issue `PLAY`. |

## 11. Timeouts and error handling

| ID | Requirement |
| --- | --- |
| ICD-FLT-001 | Control-side command handling shall use finite communication timeouts; UART response loss shall not block UI or lighting indefinitely. |
| ICD-FLT-002 | A command timeout shall result in unknown/degraded audio state until status is reacquired rather than assuming success. |
| ICD-FLT-003 | Protocol parse errors shall be logged with sufficient diagnostic information for development without requiring a user-visible error screen. |
| ICD-FLT-004 | Unknown message types shall be ignored/rejected safely and shall not alter playback. |
| ICD-FLT-005 | Communication loss shall not disable an already-running Audio Controller session timer. |

## 12. Candidate framing for M3/M4 evaluation

The implementation milestone should evaluate a compact framed binary protocol with fields equivalent to:

```text
SOF | version | type | sequence | length | payload | integrity
```

This is a design candidate, not yet a normative wire format. A compact newline-delimited textual protocol is also acceptable for the first bring-up if it meets malformed-message handling and migration requirements. The final choice shall be recorded by ADR.