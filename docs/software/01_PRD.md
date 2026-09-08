# SleepCube Software Product Requirements Document

**Revision:** R00 draft

## 1. Purpose

SleepCube software shall provide a simple bedside experience combining ambient lighting and finite-duration sleep audio using local embedded hardware only.

## 2. Product outcomes

| ID | Requirement |
| --- | --- |
| PR-SYS-001 | The software shall provide coordinated ambient lighting, local touch interaction and sleep-audio playback as one integrated user experience. |
| PR-SYS-002 | The software shall operate without dependence on cloud services, wireless connectivity or a mobile application. |
| PR-SYS-003 | The software shall start in a deterministic, quiet state with ambient visual presentation available and audio inactive. |
| PR-SYS-004 | Lighting shall remain usable independently of audio playback state or an audio-controller fault. |
| PR-UI-001 | The primary interaction model shall remain minimal and suitable for low-light bedside use. |
| PR-UI-002 | The user shall be able to start/stop audio and adjust audio volume and ambient brightness locally. |
| PR-LGT-001 | The software shall produce a warm, smooth ambient-light presentation without abrupt user-visible transitions during normal operation. |
| PR-AUD-001 | The software shall play preloaded sleep-audio content without requiring track selection during normal use. |
| PR-AUD-002 | Playback shall start, stop and change volume without objectionable clicks, pops or abrupt level changes attributable to software control. |
| PR-SES-001 | Every audio playback session shall be finite; continuous indefinite playback shall not be a normal operating mode. |
| PR-SES-002 | Expiry of an audio session shall stop audio without disabling ambient lighting. |
| PR-SET-001 | User-adjustable brightness and volume shall persist across normal power cycles unless a later requirement explicitly changes this behaviour. |
| PR-FLT-001 | A fault, reset or loss of communication in one controller shall not cause uncontrolled or indefinite audio playback. |
| PR-FLT-002 | The system shall recover to a defined usable state after either controller restarts. |

## 3. Reference implementation boundary

Software R00 is verified on:

- Waveshare ESP32-C6-Touch-LCD-1.47 as Control Controller;
- M5Stack AtomS3 Lite as Audio Controller;
- M5Stack ATOMIC Speaker Base (NS4168) as reference audio-output hardware;
- a wired local inter-controller UART link.

These devices are reference platforms for R00 rather than requirements on a future production hardware design.

## 4. Out of scope for software R00

- enclosure geometry and industrial design;
- production PCB architecture;
- regulatory certification;
- production EMC qualification;
- speaker/enclosure acoustic design targets;
- wireless control;
- mobile application;
- user-selectable track/library management;
- manufacturing test implementation beyond what is needed to validate the software baseline.