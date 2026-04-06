# SleepCube Compliance and Traceability Matrix

## Rev 01

| FR ID | FR Summary | TR IDs | Status | Coverage Notes |
| --- | --- | --- | --- | --- |
| FR-SC-01 | Indoor residential bedroom use | TR-P-02 | Implemented | Product assumptions and power budget remain aligned. |
| FR-SC-02 | Stable operation on a flat surface | TR-M-04 | Implemented | Mechanical requirement unchanged from Rev 00. |
| FR-SC-03 | Suitable for low ambient light conditions | TR-P-02 | Partial | Product direction aligns; final dark-room tuning still needs verification on target hardware. |
| FR-A-01 | Preloaded audio playback | TR-SC-01 | Implemented | MP3 playback from SD with SPIFFS fallback is present. |
| FR-A-02 | Automatic looping | TR-A-10 | Implemented | Playback task reopens the preferred file while playback remains enabled. |
| FR-A-03 | No track selection required | TR-A-11 | Implemented | No track-selection UI is exposed. |
| FR-A-04 | No audible glitches or artifacts | TR-A-12 | Partial | Basic playback works, but resampling and long-run verification remain open. |
| FR-A-05 | Start audio from touch UI | TR-UI-01 | Implemented | Touch ambient screen now emits audio-toggle actions. |
| FR-A-06 | Stop audio from touch UI | TR-UI-01 | Implemented | Touch audio toggle requests playback stop with fade-out. |
| FR-A-07 | Start without audible transients | TR-A-05 | Partial | TX enable path is validated, but explicit fade-in is still limited. |
| FR-A-08 | Stop without audible transients | TR-A-05 | Implemented | Manual stop now uses fade-out before mute. |
| FR-A-09 | Smooth start/stop level ramp | TR-A-06 | Partial | Stop fade is implemented; start ramp still needs a dedicated fade-in path. |
| FR-A-10 | Increase volume | TR-A-13 | Implemented | Touch and service paths support positive volume steps. |
| FR-A-11 | Decrease volume | TR-A-13 | Implemented | Touch and service paths support negative volume steps. |
| FR-A-12 | Barely audible minimum volume | TR-A-02 | Partial | Step control exists; acoustic verification remains open. |
| FR-A-13 | Smooth volume changes | TR-A-14 | Partial | Live volume updates are now applied during playback; smoothing remains coarse. |
| FR-A-14 | Retain last volume across normal power cycles | TR-DS-04, TR-DS-05 | Implemented | Volume is persisted in NVS. |
| FR-A-15 | Sleep-suitable maximum volume | TR-A-01, TR-P-04 | Partial | Max bound exists in software scale; final acoustic validation remains open. |
| FR-A-16 | Playback always uses sleep timer | TR-A-15 | Implemented | Playback enable starts a mandatory timer. |
| FR-A-17 | Predefined sleep timer duration | TR-A-07, TR-A-08 | Implemented | Current firmware uses a fixed 30-minute timer target. |
| FR-A-18 | No continuous playback mode | TR-A-16 | Implemented | No UI or control path bypasses the timer. |
| FR-A-19 | Stop automatically at timer expiry | TR-A-08, TR-A-09 | Implemented | Timer expiry initiates fade-out and disables playback. |
| FR-A-20 | Gradual fade-out at timer expiry | TR-A-06, TR-A-09 | Implemented | Timer expiry uses a long fade-out path. |
| FR-A-21 | Timer does not affect lighting | TR-SC-02 | Implemented | Timer expiry only affects audio state. |
| FR-L-01 | Warm ambient illumination through dedicated light | TR-L-01, TR-SC-01 | Implemented | LED strip lighting engine provides warm RGB ambient output. |
| FR-L-02 | Combined ambient presentation comfortable in dark rooms | TR-L-02, TR-L-03, TR-L-04 | Partial | Dedicated light and LCD ambient visuals exist; final comfort tuning remains open. |
| FR-L-03 | Coordinated ambient style between display and dedicated light | TR-L-07 | Partial | LCD now reacts to audio/brightness state, but full shared animation model remains future work. |
| FR-L-04 | Adjust ambient brightness | TR-L-06 | Implemented | Touch UI and service API adjust brightness. |
| FR-L-05 | Smooth brightness changes | TR-L-05 | Implemented | Light service ramps brightness over time. |
| FR-L-06 | Very low brightness setting | TR-L-03 | Implemented | Minimum brightness clamp exists and low-light path is available. |
| FR-L-07 | Retain last brightness across normal power cycles | TR-DS-04, TR-DS-05 | Implemented | Brightness is persisted in NVS. |
| FR-L-08 | Sleep-suitable maximum brightness | TR-L-02 | Implemented | LED engine uses a configurable brightness cap. |
| FR-L-09 | Avoid abrupt visual disturbances | TR-L-04, TR-L-05 | Partial | Lighting transitions are smooth; LCD transition polish still needs refinement. |
| FR-L-10 | Gradual ambient fade behavior | TR-L-05 | Implemented | Lighting path uses ramping and easing. |
| FR-UI-01 | Integrated touch display interface | TR-SC-01, TR-UI-01, TR-UI-04 | Implemented | Waveshare LCD/touch backend is active on the primary board profile. |
| FR-UI-02 | Minimal user-facing controls | TR-UI-02 | Implemented | Primary UI exposes only audio toggle, volume, and brightness. |
| FR-UI-03 | Controls for audio toggle, volume, brightness | TR-UI-03 | Implemented | Ambient screen now maps those control functions. |
| FR-UI-04 | Sleep timer starts with audio playback | TR-A-17 | Implemented | Timer starts when playback is enabled. |
| FR-UI-05 | Display contributes ambience while staying dark-room safe | TR-L-07, TR-UI-04, TR-UI-05 | Partial | Ambient LCD behavior exists; dark-room luminance verification remains open. |
| FR-UI-06 | No wireless required | TR-SC-03 | Implemented | No wireless dependency exists in runtime behavior. |
| FR-UI-07 | No mobile application required | TR-SC-03 | Implemented | No paired-app dependency exists. |
| FR-UI-08 | Consistent and predictable control behavior | TR-SC-03, TR-UI-03 | Partial | Event flow is deterministic; final UX semantics still need refinement. |
| FR-UI-09 | No separate light on/off control required | TR-UI-06 | Implemented | Primary touch UI and app-core event model no longer expose light toggle. |
| FR-P-01 | External low-voltage DC power source | TR-P-01, TR-SC-01 | Implemented | Unchanged from Rev 00. |
| FR-P-02 | Defined default state on power-up | TR-P-03, TR-SC-03 | Partial | Startup behavior is deterministic, but final ambient-default verification on target hardware remains open. |
| FR-P-03 | Default startup state: ambient on, audio off | TR-P-03 | Implemented | Light service starts enabled while audio starts disabled. |
| FR-P-04 | Same default after uncontrolled interruption | TR-SC-03 | Partial | Logic is deterministic; power-interruption recovery is not yet fully verified. |
| FR-P-05 | No unintended sound or abrupt visual output at power-up | TR-P-05 | Partial | Audio mute path is present; full startup-flash suppression on display + LED path remains to be verified. |
| FR-P-06 | Audio and lighting operate independently | TR-SC-01, TR-SC-02 | Implemented | Audio timer/stop does not disable ambient lighting. |
| FR-M-01 | Cubic form factor | TR-M-05 | Planned | Product-level mechanical target remains open beyond firmware scope. |
| FR-M-02 | Side length approximately 50-100 mm | TR-M-06 | Planned | Mechanical implementation remains future hardware work. |
| FR-M-03 | Enclosure diffuses light | TR-M-07 | Planned | Mechanical implementation remains future hardware work. |
| FR-M-04 | Stable on flat surface | TR-M-04 | Planned | Mechanical implementation remains future hardware work. |
| FR-M-05 | No bright status indicators during sleep use | TR-UI-05 | Partial | Primary UI is ambient-first, but final indicator policy still needs validation. |
| FR-M-06 | Safe surface temperature | TR-M-01, TR-M-02 | Planned | Requires hardware validation. |
| FR-Q-01 | No audible hiss at typical sleep volume | TR-A-03 | Partial | Pipeline is validated functionally; acoustic quality verification remains open. |
| FR-Q-02 | No noticeable distortion at intended max volume | TR-A-04 | Partial | Pipeline is validated functionally; acoustic quality verification remains open. |
| FR-Q-03 | No audible mechanical or electrical noise during operation | TR-M-03 | Planned | Requires integrated hardware validation. |

## Gaps and Follow-Ups

| Category | Item |
| --- | --- |
| Partial Implementation | Explicit audio fade-in remains less mature than stop/timer fade-out. |
| Partial Implementation | LCD and LED strip are only partially synchronized; a shared animation-state model is still needed. |
| Verification | Acoustic, dark-room luminance, thermal, and startup-flash behavior still need target-hardware verification. |
| Future Work | Mechanical requirements remain product targets outside current firmware scope. |
