# SleepCube Software Verification and Validation Matrix

**Revision:** R01 development

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
| SRS-SYS-004 | INS, HWT | Display, touch and external COB-light drivers/tasks exist only on Control and remain operational during Audio faults |
| SRS-SYS-005 | FI, HWT | Disconnect/reset Audio Controller; UI and lighting remain usable |
| SRS-STA-001 | HWT, LOG | Control boot with Audio absent reports unknown/unavailable; no false playing state |
| SRS-STA-002 | HWT, LOG | Normal startup and power-cycle reconciliation remain silent/STOPPED without issuing PLAY |
| SRS-STA-003 | HWT | Reset controllers independently in both boot orders |
| SRS-STA-004 | IT, FI | Reset Audio Controller; Control reacquires status without reset |
| SRS-STA-005 | HWT | Reset Audio Controller while playing; output returns stopped/safe |
| SRS-UI-001 | MAN | Rest view is default steady-state screen |
| SRS-UI-002 | MAN, IT | Short tap toggles audio request and confirmed state updates correctly |
| SRS-UI-003 | MAN | Long press enters direct volume/brightness adjustment |
| SRS-UI-004 | MAN | Adjustment view exits after inactivity timeout |
| SRS-UI-005 | FI, MAN | Brightness remains adjustable with Audio Controller disconnected |
| SRS-UI-006 | INS, MAN | Rest tap and one temporary direct-adjustment view cover normal operation without nested menus |
| SRS-LGT-001 | HWT, MAN | Continuous warm ambient effect rendered on external LEDs |
| SRS-LGT-002 | MAN | Brightness change visually smooth over full supported range |
| SRS-LGT-003 | HWT, MAN | Display/backlight and external strip respond together to selected brightness and audio-state effects |
| SRS-LGT-004 | FI | Audio stop/timeout/reset does not disable lighting engine |
| SRS-LGT-005 | INS, HWT | Transient visual response is driven by semantic state events; protocol carries no audio samples |
| SRS-LGT-011 | INS, HWT | Build/resource inspection confirms GPIO8, 3 kHz, 13-bit LEDC timer 1/channel 1 and display timer 0/channel 0; scope confirms carrier |
| SRS-LGT-012 | UT, HWT | Host boundary mapping verifies internal off and nonzero limits; target measures reset-low plus GUI minimum, midpoint and maximum at the expected active floor, intermediate range and 80% cap |
| SRS-LGT-013 | UT, HWT, MAN | Host saturation/sub-floor cases pass; scope and visual checks confirm fades below floor and effects never exceed cap |
| SRS-LGT-014 | UT, INS, HWT | Invalid floor/cap maps off and service rejects it after forcing off; reset/boot and update-fault checks leave GPIO8 low |
| SRS-AUD-001 | INS | No control-side remote filesystem/decoder/I2S API exposed through audio service |
| SRS-AUD-002 | IT | PLAY, STOP and SET_VOLUME operate across UART |
| SRS-AUD-003 | IT | Control can distinguish unavailable, stopped and playing |
| SRS-AUD-004 | HWT, MAN | Start/stop produce controlled gain transitions without obvious software-induced pop/click |
| SRS-AUD-005 | INS, HWT, MAN | 250 ms volume transition is implemented; 25/50/75/100% and slider changes are artifact-free in target listening |
| SRS-AUD-006 | HWT | End-of-content behaviour continues intended presentation without user intervention |
| SRS-AUD-007 | FI, INS | Suspend/disrupt Control-side servicing; active audio buffers remain local to Audio Controller |
| SRS-AUD-008 | INS, HWT | Canonical 44.1 kHz/16-bit mono PCM WAV mounts, validates and plays from `/sleepcube/audio.wav` |
| SRS-AUD-009 | FI, LOG | Missing card/file and malformed/unsupported WAV remain silent and produce actionable diagnostics |
| SRS-SES-001 | INS, IT | Invalid/unbounded PLAY is impossible/rejected |
| SRS-SES-002 | HWT | Nominal session duration configured to 60 min; M8-END-001 exercises the same finite 3,600-second value through the distributed facade |
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
| UX-UI-001 | HWT, MAN | Product boot and adjustment timeout settle in the ambient rest view |
| UX-UI-002 | HWT, MAN | Short rest-view taps request PLAY/STOP and confirmed responses follow authoritative Audio state |
| UX-UI-003 | HWT, LOG | Stationary long press enters adjustment view; M7 trace measured 1,020 ms |
| UX-UI-004 | HWT, LOG | Significant motion cancels the whole gesture; trace contains `gesture_cancel` and `click_suppressed` with no toggle event |
| UX-UI-005 | MAN | Rest view remains the useful ambient presentation rather than a menu |
| UX-UI-006 | HWT, MAN | Confirmed PLAY/STOP state changes produce synchronized, decaying display/strip responses |
| UX-UI-010 | HWT, MAN | Adjustment view exposes volume and brightness directly without nested menus |
| UX-UI-011 | HWT, MAN | Spatially distinct controls respond to direct touch/drag interaction |
| UX-UI-012 | HWT, MAN | Controls open at the last confirmed volume and current selected brightness |
| UX-UI-013 | HWT, LOG | Adjustment view returns to rest after approximately 3 s inactivity |
| UX-UI-014 | HWT, MAN | Entry and exit transitions are smooth rather than abrupt |
| UX-UI-015 | FI, HWT | Brightness remains usable while the Audio return path is disconnected |
| UX-UI-016 | FI, HWT | During Audio unavailability, volume input is ignored and the last confirmed value remains displayed |
| UX-AUD-001 | IT, MAN | UI distinguishes authoritatively confirmed PLAYING and STOPPED states |
| UX-AUD-002 | IT, FI | Transmit/request state alone does not create confirmed-playing presentation |
| UX-AUD-003 | FI, MAN | Link loss selects neutral/degraded presentation without false playback feedback |
| UX-AUD-004 | HWT, MAN | M6 timeout returns to STOPPED with only a transient lighting response and unchanged selected brightness |
| UX-LGT-001 | HWT, MAN | Ambient display and strip modulation is smooth and suitable in dark-room observation |
| UX-LGT-002 | HWT, MAN | Display and COB strip track brightness changes together while preserving the unchanged GUI layout/gestures and nonzero minimum behavior |
| UX-LGT-003 | MAN | Rest and adjustment operation leave no persistent bright status indicator |
| UX-LGT-004 | HWT, MAN | PLAY/STOP duty responses decay automatically and never exceed the configured COB duty cap |
| UX-STA-001 | HWT, MAN | Product startup shows the ambient view and remains silent/STOPPED |
| UX-STA-002 | FI, HWT | Audio absence does not block rest view or brightness control |
| UX-STA-003 | FI, IT | UART recovery reacquires STOPPED state without Control reset or PLAY replay |

## 4. Development milestone evidence

The following evidence supports milestone progression. Historical R00 entries
retain their original scope; they do not verify the replacement R01 COB output.

| Evidence ID | Scope | Result | Record |
| --- | --- | --- | --- |
| M1-BLD-001 | Clean ESP32-S3/8 MB Audio Controller build and image inspection | PASS | `verification/2026-09-16_M1_audio_bringup.md` |
| M1-HWT-001 | AtomS3 Lite boot/logging, 10 silent resets and 20 finite diagnostic sequences | PASS — hash-verified flash, complete application log, 10/10 silent resets to `READY`, and 21/20 exact-count retained-format sequences plus final-image regression | `verification/2026-09-16_M1_audio_bringup.md` |
| M1-MAN-001 | Speaker noise, distortion, dropout and transient listening assessment | PASS — clear tones, no distortion/clicks/pops, and quiet noise-free silent portions at approximately 20–30 cm using laptop USB power | `verification/2026-09-16_M1_audio_bringup.md` |
| M2-BLD-001 | Normal 30-minute and separate finite 60-minute Audio Controller builds; deterministic content generation | PASS — both ESP32-S3/8 MB images build at 314,352 bytes with valid image integrity; generated WAV format verified | `verification/2026-09-16_M2_standalone_audio.md` |
| M2-HWT-001 | Speaker Base FAT16 mount, canonical WAV validation, standalone controls, looping, fades, volume persistence and normal finite timeout | PASS — silent `READY`, clean loop/start/stop, 25/50/75/100% levels, NVS reset restoration and 1,800-second timeout to silent `STOPPED`; 79,381,504 frames / 317,526,016 bytes with no logged stream or runtime fault | `verification/2026-09-16_M2_standalone_audio.md` |
| M2-MAN-001 | Stored-audio noise, distortion, loop-boundary and transition assessment on supplied 1 W/8 ohm speaker | PASS — no persistent stopped-state noise, distortion, loop artifact, click or pop; manual and timeout fades clean using laptop USB power, with endurance listening at approximately 30 cm | `verification/2026-09-16_M2_standalone_audio.md` |
| M2-END-001 | Separate finite 60-minute Audio Controller endurance run | PASS — 60 loops, 158,761,472 frames / 635,045,888 bytes, finite timeout to `STOPPED`, no logged stream/runtime fault and no reported audible artifact | `verification/2026-09-16_M2_standalone_audio.md` |
| M2-FI-001 | Representative missing-storage and unsupported-content startup faults | PASS — absent card produced actionable `ESP_ERR_TIMEOUT`; valid 8-bit WAV produced actionable `ESP_ERR_NOT_SUPPORTED`; both disabled playback and remained silent | `verification/2026-09-16_M2_standalone_audio.md` |
| M2-FUP-001 | 10 W/4 ohm TEBM compatibility and intended sleep-music follow-up | PASS WITH ACCEPTED LIMITATION `AUD-ISS-001` — clean TEBM tone playback at 25%/50% and materially improved music at approximately 30 cm; no click, pop, dropout or overdrive. Low-level signal-correlated noise was later reported on three speakers; its physical cause remains unresolved, but M8-AUD-002 records the evidence-backed R00 acceptance | `verification/2026-09-18_TEBM_music_followup.md`, `issues/AUD-ISS-001-playback-noise.md` |
| M3-UT-001 | Shared COBS/CRC parser and resynchronization host tests | PASS — maximum-payload round trip; bad COBS/version/length/CRC, incomplete/overlong frames, timeout and delimiter recovery | `verification/2026-09-21_M3_uart_transport.md` |
| M3-BLD-001 | Separate ESP32-S3 Audio and ESP32-C6 Control transport builds | PASS — both 8 MB USB-console builds; Control M3 defaults preserve UI/light and disable old local audio | `verification/2026-09-21_M3_uart_transport.md` |
| M3-FLS-001 | M3 Atom flash/image identity and complete application boot | PASS — ESP32-S3 v0.2/8 MB identified; writes hash-verified; post-reset log shows valid card/content and silent `STOPPED`/`READY` | `verification/2026-09-21_M3_uart_transport.md` |
| M3-FLS-002 | Waveshare M3 flash/image identity | PASS after RESET restored USB enumeration — ESP32-C6FH8 v0.2/8 MB identified; bootloader, partition table and 595,200-byte application writes hash-verified | `verification/2026-09-21_M3_uart_transport.md` |
| M3-IT-001 | Physical bidirectional PING/PONG, malformed/truncated injection and post-error recovery | PASS — both sides matched at least 21 PONGs; independent resets triggered bad-CRC/truncated probes in both directions, both receivers rejected them and resumed valid matching | `verification/2026-09-21_M3_uart_transport.md` |
| M3-RST-001 | Independent controller reset and startup containment | PASS for both independent reset directions — transport recovered; Atom complete boot/card log reached silent `STOPPED`, and user later confirmed speaker(s) stayed silent after reset | `verification/2026-09-21_M3_uart_transport.md` |
| M3-LNK-001 | Sustained nonresponding Control peer and recovery | PASS — Atom logged nine missed PONG rounds while Waveshare was held in reset, remained stable, and resumed matches after Control restart; no physical open-circuit test claimed | `verification/2026-09-21_M3_uart_transport.md` |
| M3-MAN-001 | Retained Waveshare UI and external-light observation with M3 transport active | PARTIAL — user reports screen normal; no external LED strip was connected during M3, so external-light behavior was unverified | `verification/2026-09-21_M3_uart_transport.md` |
| M4-UT-001 | Semantic payloads, finite bounds, session identity, idempotency and events | PASS — host tests cover HELLO/INFO, GET_STATUS, PLAY/STOP/SET_VOLUME, invalid durations/volume, duplicate PLAY suppression, ID reuse/stale/session rejection, reset reconciliation, malformed/unsupported input and TIMER_EXPIRED encoding | `verification/2026-09-23_M4_semantic_audio_protocol.md` |
| M4-BLD-001 | Separate semantic Audio and UI/light-safe Control target builds | PASS — clean ESP32-S3 and ESP32-C6/8 MB builds from tracked defaults; final deployed Control image includes reset-safe USB harness fix `3908d18` | `verification/2026-09-23_M4_semantic_audio_protocol.md` |
| M4-IT-001 | Physical semantic commands, status/events, finite timeout and idempotency | PASS — PLAY/STOP, 25/50/75/100% volume and status correlated; exact PLAY replay returned DUPLICATE without deadline extension; TIMER_EXPIRED reached silent STOPPED | `verification/2026-09-23_M4_semantic_audio_protocol.md` |
| M4-RST-001 | Both boot orders and independent Control/Audio reset during playback | PASS — Control reset did not interrupt or extend Audio deadline; Audio reset stopped safely, changed session identity, and Control reconciled without replay; both boot orders synchronized by HELLO/GET_STATUS | `verification/2026-09-23_M4_semantic_audio_protocol.md` |
| M4-MAN-001 | Semantic playback transitions and retained UI/light coexistence | PASS — timeout/manual-stop/reset behavior was audibly clean and ended silent; display and GPIO8 LED strip remained operational throughout | `verification/2026-09-23_M4_semantic_audio_protocol.md` |
| M5-BLD-001 | Control-side remote facade, authoritative UI events and M5 target configuration | PASS — strict inherited protocol tests pass; final correction `31d8221` clean-builds for ESP32-C6/8 MB with local audio disabled; retained local backend also compiles | `verification/2026-09-23_M5_control_audio_facade.md` |
| M5-FLS-001 | M5 Waveshare flash, boot and initial authoritative reconciliation | PASS — final app `31d8221` writes are hash-verified; UI/light start and remote STOPPED/50%/no-fault state reconciles without automatic PLAY | `verification/2026-09-23_M5_control_audio_facade.md` |
| M5-IT-001 | Product touch PLAY/STOP/volume through the remote facade and authoritative feedback | PASS — finite PLAY and STOP complete without timeout; volume changes are serialized, confirmed and audibly artifact-free; corrected STOP response begins at authoritative STOPPING/fade onset | `verification/2026-09-23_M5_control_audio_facade.md` |
| M5-FI-001 | Audio unavailable during active playback and post-restart reconciliation | PASS — Atom reset stops output; unavailable tap produces no false PLAY; display/GPIO8 brightness remains responsive; new Audio session reconciles to STOPPED without replay | `verification/2026-09-23_M5_control_audio_facade.md` |
| M5-MAN-001 | Retained rest/adjustment UX and cross-domain presentation | PASS — PLAY/STOP audio and light feedback are synchronized; loudness follows the volume slider without artifacts; display and GPIO8 strip follow brightness smoothly and in sync | `verification/2026-09-23_M5_control_audio_facade.md` |
| M6-UT-001 | Persisted-setting bounds plus retained protocol safety regression | PASS — strict C11/UBSan frame, semantic protocol and Control setting-policy tests pass, including out-of-range brightness boundaries | `verification/2026-09-23_M6_session_fault_integration.md` |
| M6-BLD-001 | M6 production, timer-test, Audio and retained legacy Control configurations | PASS — clean builds pass for ESP32-C6/8 MB and ESP32-S3/8 MB; final Control `9e28ab7` product defaults remain 1,800 s and the explicit test overlay is 30 s | `verification/2026-09-23_M6_session_fault_integration.md` |
| M6-HWT-001 | Finite product-facade timeout, fade and authoritative presentation | PASS — accelerated 30 s path entered TIMER_EXPIRED/STOPPING, completed its five-second fade to silent STOPPED in approximately 30.1 s total, and produced synchronized transient stop feedback without changing selected brightness | `verification/2026-09-23_M6_session_fault_integration.md` |
| M6-FI-001 | Physical UART return-path loss during active playback and recovery | PASS — three bounded status attempts detected loss; UI/brightness stayed responsive; Audio stopped on its independent deadline; reconnect reconciled authoritative TIMER_EXPIRED/STOPPED without PLAY replay | `verification/2026-09-23_M6_session_fault_integration.md` |
| M6-RST-001 | Control reset during active playback through product facade | PASS — Audio continued without restart/level change to its existing deadline; new Control session synchronized to the existing Audio session and STOPPED state without extending or replaying PLAY | `verification/2026-09-23_M6_session_fault_integration.md` |
| M6-SET-001 | Distributed volume/brightness persistence and stopped startup | PASS — after normal dual-controller power cycle, Control restored brightness 42%, Audio restored volume 63%, and both reconciled stopped with silent output; invalid-bound behavior is additionally covered by M6-UT-001 and retained M2 target evidence | `verification/2026-09-23_M6_session_fault_integration.md` |
| M7-BLD-001 | Gesture hardening, LED-floor correction, strict host regression and product/trace Control builds | PASS — strict C11/UBSan frame, semantic-protocol and settings-policy tests pass; exact `65ffc61` ESP32-C6/8 MB product and trace images clean-build at 604,032 and 604,704 bytes | `verification/2026-09-23_M7_ux_lighting.md` |
| M7-HWT-001 | Rest gestures, adjustment interaction, confirmed audio presentation and coordinated lighting | PASS — PLAY/STOP responses are synchronized; moved gestures are suppressed; 1,020 ms long press, approximately 3 s return, direct sliders, preserved values and normal rest presentation pass | `verification/2026-09-23_M7_ux_lighting.md` |
| M7-MAN-001 | Dark-room display/strip behavior and low-output correction | PASS — display retains its full range; corrected strip remains a stable modulated warm white at minimum and through STOP response, with accepted low-count animation coarseness and no colorful quantization | `verification/2026-09-23_M7_ux_lighting.md` |
| M7-FI-001 | Audio return-path loss, unavailable controls and recovery | PASS — brightness remains responsive, volume retains 63% and ignores input, rest remains neutral and silent, and reconnection recovers authoritative STOPPED without reset or replay | `verification/2026-09-23_M7_ux_lighting.md` |
| M8-AUD-001 | Complete SRS/UX/ICD coverage and release-sensitive evidence audit | PASS — all 55 active SRS, 24 UX and 29 ICD IDs are mapped and have an explicit evidence disposition | `verification/2026-09-23_M8_system_validation.md` |
| M8-UT-001 | Timing-analyzer, frame, semantic-protocol and settings-policy host regressions | PASS — three timing-analyzer cases plus strict C11/UBSan protocol and policy suites pass | `verification/2026-09-23_M8_system_validation.md` |
| M8-BLD-001 | Exact committed product, timing, endurance and Audio target builds | PASS — corrected Control checkpoint `ac421a7` clean-builds product, trace and endurance profiles for ESP32-C6/8 MB; final endurance/product target artifacts carry documentation checkpoint `b69d1a9`; deployed Audio checkpoint `057f1c5` clean-builds for ESP32-S3/8 MB; exact sizes and SHA-256 identities are recorded | `verification/2026-09-23_M8_system_validation.md` |
| M8-TIM-001 | Nominal touch acceptance and complete semantic-command acknowledgement timing | PASS — 20 corrected samples: touch maximum 0.453 ms against 100 ms; user event to correlated acknowledgement maximum 59.710 ms against 250 ms; UART RTT maximum 51.870 ms | `verification/2026-09-23_M8_system_validation.md` |
| M8-RST-001 | Five independent resets of each controller, including active playback | PASS — stopped resets remained silent/STOPPED; Control reset preserved the existing Audio session without replay; Audio reset stopped safely, changed session and remained STOPPED; no automatic PLAY | `verification/2026-09-23_M8_system_validation.md` |
| M8-DEF-001 | Product isolation from M4 semantic diagnostic console | PASS — initial reset testing exposed host serial bytes issuing unintended commands; correction `ac421a7` makes the console M4-only, and the corrected timing/reset run reproduced monitor recovery without command injection or persisted-volume mutation | `verification/2026-09-23_M8_system_validation.md` |
| M8-CNT-001 | Missing canonical content and declared-data-beyond-EOF target faults | PASS — both boots produced actionable diagnostics, silent `FAULT` with playback disabled, continued UART/UI/lighting responsiveness and no panic/reboot; the exact canonical asset was hash-restored afterward | `verification/2026-09-23_M8_system_validation.md` |
| M8-PHY-001 | Final inter-controller power/wiring inspection | PASS — user explicitly confirmed TX, RX and common GND with the red 5 V contact disconnected before M8 content-fault boots | `verification/2026-09-23_M8_system_validation.md` |
| M8-AUD-002 | Evidence-backed `AUD-ISS-001` release disposition | PASS — exact canonical mono and unrelated streamed music showed comparable low-level noise in the high-volume laptop/headphone comparison; intended-distance SleepCube playback remains acceptable, so the limitation is documented and accepted for R00 without claiming root cause | `issues/AUD-ISS-001-playback-noise.md` |
| M8-END-001 | Integrated finite 60-minute playback through the distributed facade | PASS — 3,600-second PLAY at 59% advanced through retained Audio heartbeats and ended in repeated `STOPPED`, timer-expired reason 2, `fault=0`; no audible/visual artifact or target runtime fault was reported | `verification/2026-09-23_M8_system_validation.md` |
| M8-PWR-001 | Final product restore and three coordinated dual-controller power cycles | PASS — exact product image restored; each cycle booted Audio with canonical content and persisted 59% volume into silent `STOPPED`, Control polling resumed, display/strip were normal, settings stayed around 55–60%, and no automatic PLAY occurred | `verification/2026-09-23_M8_system_validation.md` |
| R00-DUR-001 | Post-M8 change of the normal product-session default to 60 minutes | PASS — exact `3348213` Control/Audio candidates booted silent and synchronized; Control requested and Audio enforced 3,600 seconds with matching countdown, then manual STOP reached clean silent `STOPPED`, reason 1, `remaining=0`, `fault=0`, without restart | `verification/2026-09-26_R00_60min_session_default.md` |
| M9-AUD-001 | Final active-document and ADR consistency audit | PASS — all active requirement/interface IDs retain explicit evidence; ADR-0002 through ADR-0008 remain accepted; no R00-critical behavioral TBD remains; `AUD-ISS-001` retains its accepted-limitation disposition | `verification/2026-09-27_M9_R00_release.md` |
| M9-BLD-001 | Exact R00 host regression and target builds | PASS — strict frame, semantic-protocol and settings-policy C11/UBSan suites plus all timing-analyzer tests pass; exact ESP32-C6/8 MB and ESP32-S3/8 MB product artifacts embed `R00` and have recorded sizes/SHA-256 identities | `verification/2026-09-27_M9_R00_release.md` |
| M9-HWT-001 | Exact R00 integrated startup, 3,600-second PLAY establishment and manual STOP smoke | PASS — both images flash with hash verification; synchronized startup is silent/STOPPED; PLAY establishes the Audio-owned 3,600-second deadline; manual STOP fades cleanly to silent `STOPPED`, reason 1, `remaining=0`, `fault=0`; display/strip remain normal | `verification/2026-09-27_M9_R00_release.md` |
| M9-REL-001 | R00 document promotion and release tag | PASS — active document headers are R00, implementation documentation matches the release path, superseded Control-local audio is absent from the active graph, and annotated tag `software-r00` identifies the release | `verification/2026-09-27_M9_R00_release.md` |
| M10-UT-001 | R01 COB mapping and safety policy host tests | PASS — strict C11/UBSan tests cover 0%, 1%, midpoint, 100%, sub-floor fade, upper/lower saturation, non-finite input and invalid floor/cap fail-off | `verification/2026-10-19_M10_cob_pwm_implementation.md` |
| M10-REG-001 | Retained settings, protocol and timing-analyzer host regressions | PASS — strict settings/frame/semantic suites and all timing-analyzer unit tests pass unchanged | `verification/2026-10-19_M10_cob_pwm_implementation.md` |
| M10-BLD-001 | Clean ESP-IDF v6.0 R01 Control build and configuration/source inspection | PASS — ESP32-C6/8 MB image builds at 586,640 bytes, embeds `R01-dev`, selects GPIO8/3 kHz/5–80%, uses distinct LEDC resources and contains no Control RGB/RMT implementation or dependency | `verification/2026-10-19_M10_cob_pwm_implementation.md` |
| M10-HWT-001 | USB-powered Waveshare GPIO8 reset level, carrier frequency and duty measurements before 12 V | PASS — exact R01 image flashed/booted; scope shows clean stable 0–3.3 V PWM at 3.012 kHz (332 µs), GPIO8 at 0 V on reset, 73.5–80% at maximum, a minimum just above the 5% floor, approximately ±7.5 duty points at the midpoint setting, responsive adjustment and PLAY/STOP pulses. ADR-0010 confirms that normal GUI operation has no 0% selection | `verification/2026-10-19_M10_cob_pwm_implementation.md` |
| M10-IT-001 | 12 V COB behavior, effects, GUI/audio regression and Audio-disconnected operation | PENDING — no powered-strip observation has been performed | `verification/2026-10-19_M10_cob_pwm_implementation.md` |
| M10-PWR-001 | Final PD-trigger/DC-DC integrated power-cycle smoke with USB disconnected | PENDING — run only after development topology passes | `verification/2026-10-19_M10_cob_pwm_implementation.md` |

Developmental issue `AUD-ISS-001` is not a failure of the M3/M4 transport or M5
facade gates. M8 closes its release-blocking status through an explicit accepted
R00 limitation supported by representative intended-distance listening, three
speaker loads and exact-content reference audition. The physical root cause is
not claimed resolved; retain it as a future audio-quality improvement item. See
`issues/AUD-ISS-001-playback-noise.md`.

M1-BLD-001 is structural evidence toward SRS-SYS-001/SRS-SYS-003 only. It does
not verify storage, decoding, product fades, session timing or the 60-minute
SRS-AUD-010 endurance requirement.

## 5. ICD coverage

| Requirement | Method | Acceptance evidence |
| --- | --- | --- |
| ICD-PHY-001 | INS, HWT | 3.3 V full-duplex UART and common ground wiring supports bidirectional target traffic |
| ICD-PHY-002 | INS, HWT | Both endpoints configure UART1 for 115200 8N1 and exchange valid frames |
| ICD-PHY-003 | INS, HWT | GPIO16/17 and GPIO1/2 are distinct from display/touch, GPIO8 strip and I2S pins; concurrent operation passes |
| ICD-PHY-004 | INS | Final mapping is frozen in `05_ICD.md` and board configuration before integration evidence |
| ICD-PHY-005 | INS, HWT | Development inspection confirms separate controller USB power, 12 V only to COB path and external DC-DC 5 V disconnected |
| ICD-PHY-006 | INS, HWT | Continuity/inspection confirms external source reference tied explicitly to Waveshare/UART common ground before 12 V |
| ICD-PHY-007 | INS, HWT | External GPIO8 gate pull-down is present and reset/boot scope trace remains off |
| ICD-PHY-008 | INS, HWT | Final PD/DC-DC smoke uses controller 5 V inputs with computer USB power disconnected unless dual-power paths are verified |
| ICD-PHY-009 | INS | Atom Grove red 5 V conductor is disconnected and insulated in both arrangements |
| ICD-PRO-001 | INS, UT | Type catalog is semantic-only and contains no audio-sample payload path |
| ICD-PRO-002 | UT, HWT | COBS zero delimiter and message type decode provide unambiguous boundaries/types |
| ICD-PRO-003 | UT, FI | Version 1 is encoded and incompatible versions are rejected |
| ICD-PRO-004 | UT, FI | Malformed, incomplete, overlong, bad-CRC and unsupported inputs fail safe |
| ICD-PRO-005 | UT, IT | Request sequence and command ID correlate acknowledgements and authoritative status |
| ICD-PRO-006 | UT, INS | Version-1 bounded header/payload/CRC-16 plus COBS framing matches ADR-0007 |
| ICD-AUD-001 | UT, IT | PLAY includes an accepted finite duration |
| ICD-AUD-002 | UT | Missing/zero/out-of-range duration is rejected |
| ICD-AUD-003 | INS, HWT | Audio establishes its local deadline before active playback and expires independently |
| ICD-AUD-004 | UT, IT | Identical PLAY retry returns `DUPLICATE` without deadline extension |
| ICD-AUD-010 | IT, HWT | STOP requests the verified controlled fade to STOPPED |
| ICD-AUD-011 | UT, IT | STOP while stopped is safe and returns an already-satisfied result |
| ICD-AUD-020 | UT, HWT | Application and protocol use the bounded 0–100 percent logical range |
| ICD-AUD-021 | UT, FI | Wire values above 100 return `INVALID_ARGUMENT` without clamp/wrap |
| ICD-AUD-022 | IT, HWT | Result/status reports confirmed applied volume and drives the Control slider |
| ICD-SYN-001 | IT, HWT | Audio-first and Control-first boot orders synchronize |
| ICD-SYN-002 | IT, LOG | HELLO/INFO followed by GET_STATUS precedes synchronized presentation |
| ICD-SYN-003 | HWT | Audio reset during playback starts silent/STOPPED |
| ICD-SYN-004 | FI, HWT | Control reset does not cancel, restart or extend the Audio deadline |
| ICD-SYN-005 | FI, HWT | Reconnection and reset recovery issue no automatic PLAY |
| ICD-FLT-001 | FI, HWT | Three finite 250 ms attempts do not block UI or lighting |
| ICD-FLT-002 | FI, MAN | Exhausted command/health timeout selects neutral unknown/degraded state |
| ICD-FLT-003 | UT, FI, LOG | Parse errors include direction/reason/count development diagnostics |
| ICD-FLT-004 | UT, FI | Unknown type returns `UNSUPPORTED` and does not alter playback |
| ICD-FLT-005 | FI, HWT | Link loss leaves the Audio-owned finite timer active through stop |

M4–M7 evidence covers the frozen semantic protocol, framing, idempotency,
bounded timeout, reset recovery and UI integration. M8 repeats selected
release-candidate fault and timing cases; it does not redefine the ICD.

## 6. R00 release gate

Software R00 shall not be released until:

1. every non-deferred SRS requirement has passing evidence;
2. all accepted ICD requirements have passing protocol/integration evidence;
3. the 60-minute audio endurance run passes;
4. controller-reset/link-loss fault injection passes;
5. the active document set contains no unresolved TBD that affects required R00 external behaviour or safety/fault containment;
6. `AUD-ISS-001` has an explicit evidence-backed disposition (verified fix/mitigation or accepted limitation), rather than being silently treated as closed by transport or validation-tone success.

The M8 evidence review found all six conditions satisfied for entry into M9.
M9 repeated the release-sensitive host/build and exact-image smoke checks,
promoted the active document set and completed the annotated `software-r00`
release. All six R00 release conditions remain satisfied.

## 7. R01 release gate

R01 shall not receive a release tag until M10-HWT-001, M10-IT-001 and
M10-PWR-001 pass with recorded frequency/duty measurements, boot/reset behavior,
GUI/settings/audio regression, flicker/noise/thermal observations and exact
flashed artifact identity. Software-only evidence is complete; this target gate
remains open.
