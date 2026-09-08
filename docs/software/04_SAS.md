# SleepCube Software Architecture Specification

**Revision:** R00 draft

## 1. Architectural drivers

The architecture is driven by four constraints:

1. preserve the mature Waveshare UI and lighting implementation;
2. remove real-time audio generation from the Waveshare controller;
3. make audio playback independently bounded and fault-contained;
4. keep the inter-controller contract small, semantic and testable.

## 2. System decomposition

```text
SleepCube software

Control Controller (Waveshare ESP32-C6)
├── app_core / product state
├── ui_service
├── display/touch drivers
├── light_service
├── lighting engine
├── settings store
├── audio_service (control-side facade)
└── audio_link / UART transport
        │
        │ semantic command/status protocol
        ▼
Audio Controller (AtomS3 Lite)
├── protocol endpoint
├── playback/session controller
├── settings as required
├── content/storage service
├── decoder
├── audio buffer pipeline
└── I2S/audio-output driver
        │
        ▼
ATOMIC Speaker Base (NS4168)
```

## 3. Ownership rules

### 3.1 Control Controller

The Control Controller owns:

- touch interpretation;
- display state and rendering;
- external RGB lighting and effects;
- product/session presentation state;
- coordination of visual response with audio state;
- transmission of semantic audio requests;
- reconciliation of user-visible state with Audio Controller reports;
- persistence of lighting configuration.

It shall not own:

- remote audio filesystem operations;
- codec/decoder control;
- audio buffering;
- I2S timing;
- amplifier-facing output generation.

### 3.2 Audio Controller

The Audio Controller owns:

- discovery/access of preloaded audio content;
- decoding and buffering;
- I2S output;
- playback gain and fades;
- authoritative playback state;
- independent session timeout enforcement;
- safe response to malformed/unsupported audio commands;
- persistence of audio-local configuration if assigned by ADR.

It shall not own:

- main product UI;
- main RGB lighting;
- overall visual product state;
- track-selection UX.

## 4. Control-side layering

The existing `app_core` event-driven structure is retained.

Target dependency direction:

```text
UI ──events──> app_core ──commands──> audio_service ──> audio_link
                     └──────────────> light_service

Audio link ──status/events──> audio_service/app_core ──> UI + lighting response
```

`audio_service` remains the application-facing audio abstraction, but its backend changes from local player/I2S calls to the remote audio link.

Cross-domain behaviour such as an audio-start visual pulse shall be coordinated by `app_core` (or a dedicated product-state layer), not by making `audio_service` call `light_service` directly.

## 5. Audio-side layering

Recommended dependency direction:

```text
UART transport
    ↓
protocol endpoint
    ↓
playback/session controller
    ├── content service → storage
    ├── decoder
    ├── gain/fade controller
    └── audio output → I2S
```

The protocol endpoint converts validated messages into semantic playback-controller operations. Protocol parsing shall not directly manipulate decoder or I2S state.

## 6. State ownership

### 6.1 Authoritative audio state

The Audio Controller is authoritative for:

- ready/not-ready;
- stopped;
- starting (if exposed);
- playing;
- stopping/fading (if exposed);
- fault.

The Control Controller may retain a requested state while awaiting confirmation, but shall distinguish requested state from confirmed remote state where this affects UX or fault handling.

### 6.2 Session state

The Control Controller owns the user/product interpretation of a session.

The Audio Controller independently owns and enforces the playback deadline. Once a finite playback session has begun, loss of the Control Controller or UART link shall not disable that deadline.

## 7. Startup synchronization

Expected nominal sequence:

```text
Control boot                 Audio boot
    │                            │
init UI/light                   init storage/audio
    │                            │
audio state = unknown           enter STOPPED/READY
    │                            │
    ├──── status query/hello ───>│
    │<──── ready + status ───────┤
    │
reconcile UI/audio state
```

No boot-order dependency shall be required. Either controller may be reset independently.

## 8. Failure containment

| Failure | Required architectural response |
| --- | --- |
| Audio Controller absent at Control boot | UI/light remain functional; audio shown unavailable/unknown |
| Control Controller resets during playback | Audio continues only within its already-established finite timeout |
| Audio Controller resets during playback | Audio output returns safe/stopped; Control later resynchronizes |
| UART corruption | Invalid frame rejected; no uncontrolled playback action |
| Unsupported protocol version | Safe non-playing interoperability failure; diagnosable in logs |
| Audio storage/decode failure | Audio Controller enters/reports fault or stopped state; Control lighting remains operational |

## 9. Concurrency principles

- Blocking UART operations shall not stall UI or lighting update paths indefinitely.
- Audio decoding/output shall not depend on servicing UART at audio sample cadence.
- State transitions shall be serialized within each controller through a single owning task/state machine or equivalent deterministic mechanism.
- Shared mutable state between tasks shall be minimized and synchronized explicitly.

## 10. Reference GPIO architecture

R00 intends to use two dedicated Waveshare GPIOs currently available for the UART link (GPIO7/GPIO8) and the AtomS3 Lite exposed G1/G2 pins, plus common ground. Exact TX/RX mapping is an implementation detail to be frozen in the ICD/reference hardware documentation before integration testing.

## 11. Decisions requiring ADRs

Accepted architecture decisions are recorded separately, including:

- two-controller partition;
- UART as inter-controller transport;
- semantic rather than low-level remote-audio API;
- independent Audio Controller timeout enforcement;
- retention/migration of existing Control Controller UI and lighting implementation.

Future ADR candidates include audio codec/storage policy, protocol framing/checksum, volume persistence ownership and degraded-state UX.