# SleepCube Software Verification and Validation Matrix

**Revision:** R00 draft

## 1. Verification method codes

| Code | Method |
| --- | --- |
| `UT` | Host/unit test where practical |
| `IT` | Two-controller integration test |
| `HWT` | Hardware-on-target test |
| `FI` | Fault-injection test |
| `MAN` | Manual UX/visual observation |
| `LOG` | Log/trace evidence |
| `INS` | Design/code/document inspection |

## 2. Core requirement coverage

| Requirement | Method | Acceptance evidence |
| --- | --- | --- |
| SRS-SYS-001 | INS, HWT | Two independently flashable controller firmwares with defined UART interface |
| SRS-SYS-002 | INS | Control architecture/state ownership documented and implemented |
| SRS-SYS-003 | INS, HWT | Audio storage/decode/I2S confined to Audio Controller |
| SRS-SYS-005 | FI, HWT | Disconnect/reset Audio Controller; UI and lighting remain usable |
| SRS-STA-001 | HWT, LOG | Control boot with Audio absent reports unknown/unavailable; no false playing state |
| SRS-STA-003 | HWT | Reset controllers independently in both boot orders |
| SRS-STA-004 | IT, FI | Reset Audio Controller; Control reacquires status without reset |
| SRS-STA-005 | HWT | Reset Audio Controller while playing; output returns stopped/safe |
| SRS-UI-001 | MAN | Rest view is default steady-state screen |
| SRS-UI-002 | MAN, IT | Short tap toggles audio request and confirmed state updates correctly |
| SRS-UI-003 | MAN | Long press enters direct volume/brightness adjustment |
| SRS-UI-004 | MAN | Adjustment view exits after inactivity timeout |
| SRS-UI-005 | FI, MAN | Brightness remains adjustable with Audio Controller disconnected |
| SRS-LGT-001 | HWT, MAN | Continuous warm ambient effect rendered on external LEDs |
| SRS-LGT-002 | MAN | Brightness change visually smooth over full supported range |
| SRS-LGT-004 | FI | Audio stop/timeout/reset does not disable lighting engine |
| SRS-AUD-001 | INS | No control-side remote filesystem/decoder/I2S API exposed through audio service |
| SRS-AUD-002 | IT | PLAY, STOP and SET_VOLUME operate across UART |
| SRS-AUD-003 | IT | Control can distinguish unavailable, stopped and playing |
| SRS-AUD-004 | HWT, MAN | Start/stop produce controlled gain transitions without obvious software-induced pop/click |
| SRS-AUD-005 | HWT, MAN | Volume transitions are subjectively smooth; quantitative criterion to be added after M2 |
| SRS-AUD-006 | HWT | End-of-content behaviour continues intended presentation without user intervention |
| SRS-AUD-007 | FI, INS | Suspend/disrupt Control-side servicing; active audio buffers remain local to Audio Controller |
| SRS-SES-001 | INS, IT | Invalid/unbounded PLAY is impossible/rejected |
| SRS-SES-002 | HWT | Nominal session duration configured to 30 min |
| SRS-SES-003 | FI, HWT | Disconnect/reset Control during playback; Audio Controller still stops at deadline |
| SRS-SES-004 | INS, IT | Control maintains user-visible session state but is not sole timeout authority |
| SRS-SES-005 | HWT, LOG | Timeout causes fade, stop and timeout/status event |
| SRS-SES-006 | HWT | Lighting brightness/state unchanged after audio timeout except transient UX indication |
| SRS-COM-001 | HWT | UART electrical/logical interface verified at configured baud |
| SRS-COM-002 | UT, FI | Corrupt/truncated frames rejected without unintended command execution |
| SRS-COM-003 | IT | Startup status synchronization works in both boot orders |
| SRS-COM-004 | FI | Unsupported command/version fails safe |
| SRS-COM-005 | FI, HWT | Cable/link loss does not defeat active timeout |
| SRS-COM-006 | INS | Protocol contains no audio sample transport path |
| SRS-SET-001 | HWT | Brightness survives normal power cycle |
| SRS-SET-002 | HWT | Volume survives normal power cycle |
| SRS-SET-003 | UT, HWT | Corrupt/out-of-range stored setting uses valid bounded fallback |
| SRS-SET-004 | HWT | Power interruption does not resume playback automatically |
| SRS-FLT-001 | FI | Audio fault does not stall UI/light tasks |
| SRS-FLT-002 | FI | Control reset does not cancel Audio Controller deadline |
| SRS-FLT-003 | IT, LOG | Reconnection reacquires authoritative status before synchronized presentation |
| SRS-FLT-004 | FI | Bad inputs never produce wrapped max volume or indefinite playback |
| SRS-FLT-005 | LOG | Communication faults generate actionable development logs |
| SRS-SYS-010 | HWT, LOG | Touch event accepted/posted within 100 ms under nominal load |
| SRS-COM-010 | IT, LOG | Command-to-confirmation latency ≤250 ms nominally |
| SRS-LGT-010 | HWT, MAN | No visible light animation stalls during normal UART/UI activity |
| SRS-AUD-010 | HWT, LOG | 60 min continuous playback run completes without buffer underrun |

## 3. UX coverage

| Requirement | Method | Acceptance evidence |
| --- | --- | --- |
| UX-UI-002 | MAN | Short tap toggles playback request |
| UX-UI-003 | MAN | ~1 s long press enters adjustment view |
| UX-UI-004 | MAN | Motion cancels long-press recognition |
| UX-UI-013 | MAN | Adjustment overlay returns to rest after ~3 s inactivity |
| UX-UI-014 | MAN | Overlay transition is animated rather than abrupt |
| UX-AUD-002 | FI, MAN | Lost acknowledgement does not falsely show confirmed playing |
| UX-LGT-001 | MAN | Ambient display modulation is smooth in dark-room observation |
| UX-STA-002 | FI, MAN | Audio absence does not block rest view/brightness control |

## 4. ICD coverage

All normative `ICD-*` requirements shall receive direct protocol unit or integration coverage when the final framing is frozen. M3 may use temporary bring-up tests; M4 completion requires an updated matrix with concrete frame-level test cases.

## 5. R00 release gate

Software R00 shall not be released until:

1. every non-deferred SRS requirement has passing evidence;
2. all accepted ICD requirements have passing protocol/integration evidence;
3. the 60-minute audio endurance run passes;
4. controller-reset/link-loss fault injection passes;
5. the active document set contains no unresolved TBD that affects required R00 external behaviour or safety/fault containment.