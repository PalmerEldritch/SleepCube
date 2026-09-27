# SleepCube Inter-Controller Interface Control Document

**Revision:** R00

## 1. Purpose

This document defines the logical and electrical contract between the Control Controller and Audio Controller.

This revision freezes the physical mapping, ADR-0007 transport framing and the
M4 semantic audio contract. ADR-0008 defines session identity, idempotent
command handling, retry limits and reset reconciliation.

## 2. Physical interface

| ID | Requirement |
| --- | --- |
| ICD-PHY-001 | The interface shall use 3.3 V logic-level full-duplex UART and a common ground. |
| ICD-PHY-002 | The nominal UART configuration shall be 115200 baud, 8 data bits, no parity, 1 stop bit (115200 8N1). |
| ICD-PHY-003 | R00 shall use dedicated GPIO not shared with real-time audio output, display/touch or external RGB-light timing. |
| ICD-PHY-004 | Final TX/RX GPIO assignments shall be documented before M3 integration testing. |

The selected three-wire mapping is:

| Signal | Waveshare ESP32-C6 header H1 | AtomS3 Lite Grove HY2.0-4P |
| --- | --- | --- |
| Control TX → Audio RX | Pin 5, GPIO16 / `ESP_TXD` | Yellow, GPIO2, configured RX |
| Audio TX → Control RX | Pin 7, GPIO17 / `ESP_RXD` | White, GPIO1, configured TX |
| Common ground | Pin 6, GND | Black, GND |

The Atom Grove red 5 V contact is left disconnected and insulated; neither
board's 5 V nor 3.3 V rail shall be connected across the link. Each board is
powered by its own USB-C cable. Both UART drivers may select a free peripheral
and map it to these GPIOs; the Waveshare's `ESP_TXD`/`ESP_RXD` labels describe
its header nets, not a mandatory peripheral instance. The USB Serial/JTAG
console remains separate. Verify physical header orientation before power.

## 3. Protocol principles

| ID | Requirement |
| --- | --- |
| ICD-PRO-001 | The protocol shall exchange semantic commands, responses and events; it shall not transport audio samples. |
| ICD-PRO-002 | Each message shall have an unambiguous boundary and message type. |
| ICD-PRO-003 | Each message shall carry a protocol version or be interpreted within an explicitly negotiated protocol version. |
| ICD-PRO-004 | The receiver shall be able to reject malformed, incomplete or unsupported messages without performing an unsafe action. |
| ICD-PRO-005 | Commands that change audio state shall support correlation with an acknowledgement/result or resulting authoritative status. |
| ICD-PRO-006 | Each wire packet shall use ADR-0007 version-1 COBS/zero-delimited binary framing, the bounded decoded header/payload/CRC-16 format below, and a 16-bit request sequence echoed by responses. |

Decoded packet layout, before COBS encoding:

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 1 | Protocol version (`1`) |
| 1 | 1 | Message type |
| 2 | 2 | Request/response sequence, little-endian |
| 4 | 1 | Payload length (`0`–`32`) |
| 5 | 0–32 | Payload |
| 5 + payload length | 2 | CRC-16/CCITT-FALSE, little-endian, over preceding decoded bytes |

COBS-encoded bytes are followed by one `0x00` delimiter. The maximum encoded
packet is 41 bytes including that delimiter. Invalid length, version, COBS,
CRC, overlong or time-expired partial frames shall not reach semantic handlers.
M3 diagnostic types `0x70` PING and `0x71` PONG remain reserved transport-only
values and shall not change playback state. They are not emitted by the M4
semantic implementation.

## 4. Semantic type catalog

Protocol multibyte integers are unsigned little-endian. Request sequences shall
be non-zero; sequence zero is reserved for unsolicited Audio events. A response
echoes its request sequence.

| Type | Value | Direction | Payload | Meaning |
| --- | ---: | --- | --- | --- |
| `HELLO` | `0x01` | Control → Audio | `control_session_id:u32` | Establish the current Control boot/session and request compatibility information |
| `GET_STATUS` | `0x02` | Control → Audio | none | Request an authoritative snapshot |
| `PLAY` | `0x03` | Control → Audio | common command header + `duration_seconds:u32` | Request finite playback |
| `STOP` | `0x04` | Control → Audio | common command header | Request controlled stop |
| `SET_VOLUME` | `0x05` | Control → Audio | common command header + `volume_percent:u8` | Request authoritative persisted volume |
| `INFO` | `0x81` | Audio → Control | information payload | Correlated `HELLO` response |
| `RESULT` | `0x82` | Audio → Control | result payload | Correlated state-changing-command acknowledgement/result |
| `READY` | `0x83` | Audio → Control | status payload | Audio services became command-ready; also an unsolicited event |
| `STATUS` | `0x84` | Audio → Control | status payload | Correlated `GET_STATUS` response or unsolicited material status change |
| `PLAYING` | `0x85` | Audio → Control | status payload | Authoritative transition into active playback |
| `STOPPED` | `0x86` | Audio → Control | status payload | Authoritative non-timeout stop transition |
| `TIMER_EXPIRED` | `0x87` | Audio → Control | status payload | Independent Audio deadline caused the stop transition |
| `ERROR` | `0x8f` | Audio → Control | error response or fault status payload | Malformed/unsupported request response or unsolicited subsystem fault |

No command shall expose remote decoder setup, I2S clocking or remote filesystem manipulation to normal Control Controller application code.

### 4.1 Common command header

All state-changing commands begin with:

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | `control_session_id` |
| 4 | 4 | `audio_session_id` |
| 8 | 4 | `command_id` |

`command_id` starts at one and increases monotonically within one Control
session. It shall not wrap; a new Control session shall be established first.

### 4.2 Information payload

`INFO` is 19 bytes:

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | echoed `control_session_id` |
| 4 | 4 | current `audio_session_id` |
| 8 | 1 | semantic version (`1`) |
| 9 | 2 | capabilities bit mask: bit 0 finite PLAY, bit 1 volume, bit 2 status |
| 11 | 4 | minimum accepted duration in seconds |
| 15 | 4 | maximum accepted duration in seconds |

The R00 implementation advertises durations from 1 through 3,600 seconds. The
normal product request is 3,600 seconds; shorter values are permitted for
bounded development/integration tests.

### 4.3 Status payload

All authoritative snapshots are 14 bytes:

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | `audio_session_id` |
| 4 | 1 | state code |
| 5 | 1 | confirmed volume, 0–100 percent |
| 6 | 1 | stop-reason code |
| 7 | 1 | flags; zero in semantic version 1 |
| 8 | 4 | whole seconds remaining, rounded up; zero when inactive |
| 12 | 2 | fault code; zero when no fault |

State codes are `0 NOT_READY`, `1 STOPPED`, `2 STARTING`, `3 PLAYING`,
`4 STOPPING`, and `5 FAULT`. Stop-reason codes are `0 BOOT/NONE`, `1 MANUAL`,
`2 TIMER_EXPIRED`, and `3 FAULT`. The status payload carries the `VOLUME`
semantics; semantic version 1 has no separate volume-event type.

### 4.4 Result and error payloads

`RESULT` is 20 bytes: request type (`u8`), result code (`u8`), command ID
(`u32`), then the 14-byte authoritative status snapshot. Result codes are:

| Value | Name | Meaning |
| ---: | --- | --- |
| 0 | `ACCEPTED` | Request validated and queued for the playback owner |
| 1 | `NO_CHANGE` | Requested condition was already satisfied |
| 2 | `DUPLICATE` | Same command was seen before and was not executed again |
| 3 | `NOT_READY` | Audio services cannot accept the operation |
| 4 | `INVALID_ARGUMENT` | Value is outside the bounded contract |
| 5 | `SESSION_MISMATCH` | Session identity is stale or unknown |
| 6 | `STALE_COMMAND` | Command ID precedes the latest command in this Control session |
| 7 | `COMMAND_ID_REUSED` | Latest command ID was reused with different type/payload |
| 8 | `BUSY` | Current playback state cannot accept the operation |
| 9 | `INTERNAL_ERROR` | Operation could not be queued or serviced |
| 10 | `UNSUPPORTED` | Message type is unsupported |
| 11 | `MALFORMED` | Known message has an invalid payload shape |

`ACCEPTED` is an acknowledgement of validated queueing, not a claim that an
asynchronous fade or persistent-volume write has completed. Control shall use
subsequent status/events as authoritative confirmation.

A correlated request `ERROR` is 16 bytes: offending request type (`u8`), result
code (`u8`), then the status payload. An unsolicited subsystem `ERROR` uses the
14-byte fault status payload and sequence zero.

## 5. Event semantics

Audio emits sequence-zero events for material authoritative changes. `READY`
marks transition from NOT_READY into an initialized state. `PLAYING` confirms
the active state; `STOPPED` confirms a manual/already-satisfied stop;
`TIMER_EXPIRED` distinguishes the independently enforced deadline; `STATUS`
carries transient-state or confirmed-volume changes; and `ERROR` carries a
fault snapshot. Events supplement but do not replace explicit `GET_STATUS`
reconciliation after startup, timeout or reset.

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

Semantic version 1 exposes `STARTING`, `STOPPING` and `FAULT` as defined above.
Control may present them as transitional/degraded product state, but Audio
remains authoritative.

## 7. PLAY contract

| ID | Requirement |
| --- | --- |
| ICD-AUD-001 | `PLAY` shall contain or imply a finite non-zero session duration accepted by the Audio Controller. |
| ICD-AUD-002 | A `PLAY` request with absent, zero, out-of-range or otherwise invalid duration shall be rejected. |
| ICD-AUD-003 | Successful `PLAY` shall establish the Audio Controller's independent timeout before or atomically with entry into playing state. |
| ICD-AUD-004 | Receipt of duplicate/retried `PLAY` messages shall not accidentally create an unbounded or multiplicatively extended session. Identical retries in the current session shall return `DUPLICATE` without invoking playback again. |

## 8. STOP contract

| ID | Requirement |
| --- | --- |
| ICD-AUD-010 | `STOP` shall request a controlled fade followed by stopped state. |
| ICD-AUD-011 | `STOP` received while already stopped shall be safe and may be acknowledged as already satisfied. |

## 9. Volume contract

| ID | Requirement |
| --- | --- |
| ICD-AUD-020 | Volume shall use a bounded logical range independent of amplifier implementation. R00 application-facing range is 0-100 percent. |
| ICD-AUD-021 | Out-of-range wire values shall be rejected with `INVALID_ARGUMENT`; they shall never be clamped or wrap numerically. |
| ICD-AUD-022 | The Audio Controller shall report the resulting confirmed volume after successful application or via subsequent status. |

## 10. Startup and resynchronization

| ID | Requirement |
| --- | --- |
| ICD-SYN-001 | The protocol shall not assume either controller boots first. |
| ICD-SYN-002 | After establishing communication, the Control Controller shall obtain authoritative Audio Controller status before treating audio state as synchronized. |
| ICD-SYN-003 | An Audio Controller reset shall result in safe non-playing startup state. |
| ICD-SYN-004 | A Control Controller reset during playback shall not remove or extend the Audio Controller's existing finite session deadline. |
| ICD-SYN-005 | Reconnection shall not automatically issue `PLAY`. |

Each boot generates a non-zero random `control_session_id` or
`audio_session_id` on its respective controller. `HELLO` installs the current
Control session on Audio without changing playback or its existing deadline.
Every state-changing command carries both IDs. A new Control session clears the
Audio duplicate cache but does not stop, start or extend playback. A new Audio
session invalidates outstanding Control commands; Control shall perform
`HELLO`, then `GET_STATUS`, before accepting new application requests.

Audio retains the most recently processed state-changing command for the
current Control session. An identical retry with the same ID returns
`DUPLICATE`; the same ID with a different type/payload returns
`COMMAND_ID_REUSED`; an older ID returns `STALE_COMMAND`. Only one Control
request is outstanding at a time, so retention of the latest command is
sufficient for version 1.

## 11. Timeouts and error handling

| ID | Requirement |
| --- | --- |
| ICD-FLT-001 | Control-side command handling shall use finite communication timeouts; UART response loss shall not block UI or lighting indefinitely. |
| ICD-FLT-002 | A command timeout shall result in unknown/degraded audio state until status is reacquired rather than assuming success. |
| ICD-FLT-003 | Protocol parse errors shall be logged with sufficient diagnostic information for development without requiring a user-visible error screen. |
| ICD-FLT-004 | Unknown message types shall be ignored/rejected safely and shall not alter playback. |
| ICD-FLT-005 | Communication loss shall not disable an already-running Audio Controller session timer. |

The version-1 Control client waits 250 ms for a correlated response and makes
at most three total transmissions of the exact same packet. After exhaustion,
it marks remote state unsynchronized and periodically retries `HELLO`; it does
not reconstruct or automatically reissue the timed-out state-changing command.
After compatibility is restored it requests status. UI and lighting work shall
not wait on this retry loop.

While synchronized and with no request in flight, the R00 Control client issues
a `GET_STATUS` health probe at a one-second interval. A queued application
request takes priority over starting a new probe. Failure of all three probe
attempts enters the same unsynchronized/degraded state described above; recovery
uses HELLO followed by GET_STATUS and does not replay PLAY.

## 12. M4 validation boundary

Sections 3–11 and ADR-0007/ADR-0008 define semantic version 1. The M4 Control
image exposes a USB-console development harness for bounded PLAY, STOP,
SET_VOLUME, GET_STATUS and deliberate duplicate replay. That harness is not the
product UI facade; connecting existing touch/UI behavior is M5.
