# SleepCube Software Requirements Specification

**Revision:** R00 draft

## 1. System and ownership

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-SYS-001 | PR-SYS-001 | The software shall be partitioned into a Control Controller firmware and an Audio Controller firmware communicating through a defined local interface. |
| SRS-SYS-002 | PR-SYS-003 | The Control Controller shall own overall product/session state and user-visible state. |
| SRS-SYS-003 | PR-AUD-001 | The Audio Controller shall own audio storage access, decoding, buffering, playback control, I2S generation and amplifier-facing audio output. |
| SRS-SYS-004 | PR-LGT-001 | The Control Controller shall own display rendering, touch processing, external RGB lighting and ambient-light effects. |
| SRS-SYS-005 | PR-SYS-004 | Control-side lighting operation shall not depend on successful Audio Controller initialization or continued communication with it. |

## 2. Startup and state

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-STA-001 | PR-SYS-003 | After Control Controller boot, audio shall be treated as inactive until the Audio Controller explicitly reports a ready/known state. |
| SRS-STA-002 | PR-SYS-003 | Normal startup shall not automatically begin audio playback. |
| SRS-STA-003 | PR-FLT-002 | Each controller shall initialize independently and shall not require simultaneous reset with the other controller. |
| SRS-STA-004 | PR-FLT-002 | The Control Controller shall be capable of synchronizing its audio-related state after the Audio Controller restarts. |
| SRS-STA-005 | PR-FLT-002 | The Audio Controller shall enter a safe non-playing state after its own reset unless an accepted future ADR explicitly defines resumable playback. |

## 3. User interface

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-UI-001 | PR-UI-001 | The default user interface shall use an ambient rest view rather than persistent menus or status screens. |
| SRS-UI-002 | PR-UI-002 | A normal user action from the rest view shall toggle audio playback. |
| SRS-UI-003 | PR-UI-002 | The user shall be able to adjust volume and ambient brightness from a temporary adjustment view. |
| SRS-UI-004 | PR-UI-001 | The adjustment view shall return automatically to the rest view after inactivity. |
| SRS-UI-005 | PR-SYS-004 | User interaction with brightness shall remain functional when the audio subsystem is unavailable. |
| SRS-UI-006 | PR-UI-001 | The UI shall avoid requiring multi-level menu navigation for normal operation. |

## 4. Lighting

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-LGT-001 | PR-LGT-001 | The Control Controller shall provide a continuously rendered warm ambient-light effect on the external RGB lighting output. |
| SRS-LGT-002 | PR-LGT-001 | User-requested brightness changes shall transition smoothly rather than as abrupt full-scale steps. |
| SRS-LGT-003 | PR-LGT-001 | The display/backlight presentation shall be coordinated with the ambient-light presentation. |
| SRS-LGT-004 | PR-SYS-004 | Audio start, stop, timeout or audio-link failure shall not disable the ambient-light engine. |
| SRS-LGT-005 | PR-SYS-001 | The Control Controller may provide transient visual response to audio state changes, but such response shall not require real-time audio sample transport from the Audio Controller. |

## 5. Audio control and playback

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-AUD-001 | PR-AUD-001 | The Control Controller shall control audio through semantic commands exposed by the Audio Controller and shall not manage remote filesystem, decoder or I2S implementation details. |
| SRS-AUD-002 | PR-AUD-001 | The Audio Controller shall support start and stop commands and a volume-setting command. |
| SRS-AUD-003 | PR-AUD-001 | The Audio Controller shall expose enough status for the Control Controller to distinguish at least unavailable/not-ready, stopped and playing states. |
| SRS-AUD-004 | PR-AUD-002 | Audio start and manual stop shall use controlled gain transitions to avoid abrupt software-induced level changes. |
| SRS-AUD-005 | PR-AUD-002 | User volume changes shall be applied smoothly enough to avoid objectionable discrete gain transients. |
| SRS-AUD-006 | PR-AUD-001 | End-of-content handling shall continue the intended sleep-audio presentation without requiring user track selection. Exact looping/content sequencing is TBD until the audio content format is frozen. |
| SRS-AUD-007 | PR-AUD-001 | Audio playback shall not depend on the Control Controller continuously servicing audio buffers or file data. |

## 6. Session timer

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-SES-001 | PR-SES-001 | No normal command path shall permit the Audio Controller to enter a playing state without a finite session timeout. |
| SRS-SES-002 | PR-SES-001 | The nominal software R00 playback-session duration shall be 30 minutes. |
| SRS-SES-003 | PR-SES-001, PR-FLT-001 | The Audio Controller shall independently enforce session timeout once playback has begun. |
| SRS-SES-004 | PR-SES-001 | The Control Controller shall maintain corresponding user-visible session state but shall not be the sole authority preventing indefinite playback. |
| SRS-SES-005 | PR-SES-002 | At session expiry, the Audio Controller shall perform a gradual fade-out, stop playback and report the resulting state/event to the Control Controller. |
| SRS-SES-006 | PR-SES-002 | Session expiry shall not modify Control Controller lighting state except for any transient visual indication explicitly defined by UX requirements. |

## 7. Communication

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-COM-001 | PR-SYS-001 | The controllers shall communicate using a local 3.3 V full-duplex UART link on dedicated GPIO. |
| SRS-COM-002 | PR-FLT-001 | The protocol shall provide message boundaries and detection/rejection of malformed messages. |
| SRS-COM-003 | PR-FLT-002 | The protocol shall support startup synchronization and explicit status query/reporting. |
| SRS-COM-004 | PR-FLT-002 | Receiving an unsupported command or protocol version shall not cause uncontrolled audio output. |
| SRS-COM-005 | PR-FLT-001 | Loss of the UART link after playback begins shall not prevent the Audio Controller session timeout from stopping playback. |
| SRS-COM-006 | PR-SYS-001 | Audio sample data shall not be transported over the inter-controller protocol. |

## 8. Persistence

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-SET-001 | PR-SET-001 | Ambient brightness shall persist across normal power cycles. |
| SRS-SET-002 | PR-SET-001 | Audio volume shall persist across normal power cycles. |
| SRS-SET-003 | PR-SET-001 | Persisted values shall be validated and clamped to supported ranges before use. |
| SRS-SET-004 | PR-SYS-003 | Playback state and remaining session time shall not be restored automatically after an uncontrolled power interruption in R00. |

## 9. Fault handling

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-FLT-001 | PR-FLT-001 | An Audio Controller fault shall not block the Control Controller UI task or lighting task indefinitely. |
| SRS-FLT-002 | PR-FLT-001 | A Control Controller fault or reset shall not cancel the Audio Controller's independently active finite session timeout. |
| SRS-FLT-003 | PR-FLT-002 | After communication is re-established, the Control Controller shall query or receive authoritative Audio Controller state before presenting audio state as synchronized. |
| SRS-FLT-004 | PR-FLT-001 | Malformed, partial or out-of-range control messages shall fail safe and shall not result in maximum-volume or indefinite playback. |
| SRS-FLT-005 | PR-FLT-002 | Recoverable communication faults shall be diagnosable through firmware logging without requiring user-visible debug UI. |

## 10. Performance targets

| ID | Derived From | Requirement |
| --- | --- | --- |
| SRS-SYS-010 | PR-UI-002 | Normal touch actions shall be accepted by the Control Controller within 100 ms under nominal load. |
| SRS-COM-010 | PR-UI-002 | Under nominal conditions, a user audio command shall reach the Audio Controller and receive acknowledgement/state confirmation within 250 ms. |
| SRS-LGT-010 | PR-LGT-001 | Lighting animation shall remain visually continuous during normal UART traffic and UI interaction. |
| SRS-AUD-010 | PR-AUD-001 | Audio playback shall sustain the selected content format without buffer underrun during a 60-minute validation run under nominal conditions. |

## 11. Explicitly deferred requirements

The following shall be resolved before software R00 is released but are not yet specified here:

- supported audio codec/container and sample format;
- file naming/content-discovery rules;
- exact UART packet representation and integrity mechanism;
- detailed acknowledgement/retry policy;
- quantitative audio fade durations and volume law after hardware bring-up.