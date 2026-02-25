# SleepCube -- Technical Requirements Specification (TRS)

**Document status:** v1.0\
**Derived from:** SleepCube FRS v1.0\
**Date generated:** 2026-02-24\
**Scope:** Defines technical requirements necessary to fulfill
functional requirements.\
**Implementation:** Technology-neutral unless required for verification.

------------------------------------------------------------------------

# 1. System Architecture Requirements

## TR-SC-01 -- Functional Partitioning
Derived From: FR-A-01, FR-L-01, FR-UI-01, FR-P-01, FR-P-06

The system shall be partitioned into at least the following functional
subsystems: 
- Power subsystem
- Audio playback subsystem
- Lighting subsystem
- Control & logic subsystem
- User interface subsystem

## TR-SC-02 -- Functional Independence
Derived From: FR-L-03, FR-A-21, FR-P-06

Audio and lighting subsystems shall be electrically and logically
independent such that failure or shutdown of one does not disturb the
operation of the other.

## TR-SC-03 -- Deterministic Behavior
Derived From: FR-P-02, FR-P-04, FR-UI-06, FR-UI-07, FR-UI-08

The control subsystem shall provide deterministic startup and runtime
behavior without reliance on external communication or cloud services.

------------------------------------------------------------------------

# 2. Audio Requirements

## TR-A-01 -- Maximum Loudness
Derived From: FR-A-15

Maximum sound pressure level at 0.5 m shall be ≤ 55 dBA at maximum
volume.

## TR-A-02 -- Minimum Loudness
Derived From: FR-A-12

Minimum volume setting shall produce 25--30 dBA at 0.5 m.

## TR-A-03 -- Noise Floor
Derived From: FR-Q-01

At typical sleep volume, A-weighted noise at 0.5 m with silent input
shall be ≤ 20 dBA or ≥ 50 dB SNR referenced to 40 dBA output.

## TR-A-04 -- Distortion
Derived From: FR-Q-02

THD+N at 1 kHz into nominal load at max volume shall be ≤ 3%.

## TR-A-05 -- Start/Stop Transient
Derived From: FR-A-07, FR-A-08

Speaker terminal transient shall be ≤ 50 mV peak during start/stop
events.

## TR-A-06 -- Ramping
Derived From: FR-A-09, FR-A-20

Start fade-in: 0.3--1.0 s
Stop fade-out: 0.3--1.0 s
Timer fade-out: 10--30 s

------------------------------------------------------------------------

# 3. Sleep Timer Requirements

## TR-A-07 -- Duration
Derived From: FR-A-17

Predefined sleep timer duration shall be 30 minutes.

## TR-A-08 -- Accuracy
Derived From: FR-A-17, FR-A-19

Timer accuracy shall be ±2% or ±30 seconds (whichever larger) over 0--40
°C.

## TR-A-09 -- Expiry Behavior
Derived From: FR-A-19, FR-A-20

Fade-out shall complete within 10--30 seconds after expiration.

## TR-A-10 – Continuous Playback Logic
Derived From: FR-A-02

Audio control logic shall automatically restart playback from the beginning of the available audio content within ≤ 100 ms of reaching end-of-content, without requiring user interaction.

No unintended silence longer than 100 ms shall occur between loops.

## TR-A-11 – Fixed Content Configuration
Derived From: FR-A-03

The system shall expose no user interface function for selecting individual tracks.

Audio content structure shall be treated as a single logical playback sequence.

## TR-A-12 – Playback Integrity
Derived From: FR-A-04

Under steady-state operation:

No buffer underruns shall occur at nominal input voltage range (4.75–5.25 V).

No audible dropouts >10 ms shall occur over 1-hour continuous playback.

Playback timing jitter shall not produce audible artifacts.

## TR-A-13 – Volume Step Resolution
Derived From: FR-A-10, FR-A-11

The system shall provide ≥ 8 discrete volume levels across the defined SPL range.

Adjacent volume steps shall not exceed 6 dB difference.

## TR-A-14 – Volume Transition Behavior
Derived From: FR-A-13

Volume adjustments shall ramp over 100–300 ms to prevent audible steps.

## TR-A-15 – Mandatory Timer Enforcement
Derived From: FR-A-16

Audio playback state machine shall automatically initialize a countdown timer upon entering playback state.

No control path shall allow playback without an active timer instance.

## TR-A-16 – No Continuous Mode
Derived From: FR-A-18

Firmware shall not implement a continuous playback mode.

Verification shall confirm absence of firmware state enabling indefinite playback.

## TR-A-17 – Automatic Timer Start
Derived From: FR-UI-04

Timer initialization shall occur within ≤ 100 ms of entering audio playback state.

------------------------------------------------------------------------

# 4. Lighting Requirements

## TR-L-01 -- Color Temperature
Derived From: FR-L-01

Correlated color temperature shall be 2200--2700 K.

## TR-L-02 -- Maximum Illuminance
Derived From: FR-L-02, FR-L-08

At 1.0 m distance, maximum brightness shall provide 10--30 lux.

## TR-L-03 -- Minimum Illuminance
Derived From: FR-L-02, FR-L-06

At 1.0 m distance, minimum brightness shall provide ≤ 0.3 lux.

## TR-L-04 -- Flicker
Derived From: FR-L-02, FR-L-09

Percent flicker ≤ 10% and flicker index ≤ 0.1 at all brightness
settings.

## TR-L-05 -- Transition Time
Derived From: FR-L-05, FR-L-09, FR-L-10

On/off and brightness changes shall ramp over 0.3--1.0 s.

## TR-L-06 – Brightness Control Resolution
Derived From: FR-L-04

The system shall provide ≥ 8 discrete brightness levels between minimum and maximum defined illuminance.

Brightness resolution steps shall not exceed a factor of 1.8 between adjacent levels.

------------------------------------------------------------------------

# 5. Power Requirements

## TR-P-01 -- Input Voltage
Derived From: FR-P-01

Nominal 5.0 V DC input, allowable range 4.75--5.25 V.

## TR-P-02 -- Maximum Input Power
Derived From: FR-SC-01, FR-SC-03

Worst-case simultaneous operation shall not exceed 5 W input power.

## TR-P-03 -- Idle Power
Derived From: FR-P-02, FR-P-03

Light minimum, audio off: ≤ 0.5 W\
Light off, audio off: ≤ 0.3 W

## TR-P-04 -- Audio-Only Power
Derived From: FR-A-15, FR-SC-01

Typical sleep volume with light off: ≤ 1.0 W average.

## TR-P-05 – Power-Up Output Suppression
Derived From: FR-P-05

During the first 200 ms after input voltage rises above 4.5 V:

Audio amplifier output shall remain muted.

Lighting output shall remain off until control subsystem initialization completes.

No transient light pulse >10% of maximum brightness shall occur.

------------------------------------------------------------------------

# 6. Thermal Requirements

## TR-M-01 -- Surface Temperature
Derived From: FR-M-06

Accessible surfaces shall not exceed: - 42 °C at 25 °C ambient (4-hour
steady-state)\
- 48 °C at 35 °C ambient (4-hour steady-state)

## TR-M-02 -- Local Hotspots
Derived From: FR-M-06

Hotspots shall not exceed surface limit by more than +3 °C.

------------------------------------------------------------------------

# 7. Mechanical & Acoustic Requirements

## TR-M-03 -- Mechanical Noise
Derived From: FR-Q-03

With audio muted and light at any brightness, device acoustic noise at
0.5 m shall be ≤ 20 dBA.

## TR-M-04 – Static Stability
Derived From: FR-SC-02, FR-M-04

The device shall remain stable and not tip when placed on a horizontal surface tilted up to 10° in any direction.

The device shall withstand a horizontal force of 2 N applied at the top edge without tipping.

## TR-M-05 – Dimensional Equality
Derived From: FR-M-01

All three external side lengths shall be equal within ±2 mm tolerance.

## TR-M-06 – Dimensional Envelope
Derived From: FR-M-02

External side length shall be between 50 mm and 100 mm inclusive.

## TR-M-07 – Glare Limitation
Derived From: FR-M-03

No directly visible LED die shall be observable from any normal viewing angle at 0.5 m distance.

Maximum luminance at enclosure surface shall not exceed 5,000 cd/m².

------------------------------------------------------------------------

# 8. Non-Volatile Memory Requirements

## TR-DS-04 -- Data Retention
Derived From: FR-A-14, FR-L-07

Stored parameters shall retain data ≥ 5 years without power.

## TR-DS-05 -- Write Endurance
Derived From: FR-A-14, FR-L-07

Settings storage shall support ≥ 50,000 update cycles.

------------------------------------------------------------------------

# 9. User control interfaces

## TR-UI-01 – Audio Control Interface
Derived From: FR-A-05, FR-A-06

A physical user control shall generate a deterministic control signal detectable within ≤ 50 ms by the control subsystem.

Control input shall be debounced such that unintended multiple triggers do not occur.

## TR-UI-02 – Control Count Limit
Derived From: FR-UI-02

The device shall implement no more than 6 user-accessible physical buttons.

## TR-UI-03 – Control Mapping
Derived From: FR-UI-03

Physical controls shall provide functions for:

- Audio start/stop

- Volume increase

- Volume decrease

- Light on/off

- Brightness adjustment

Each function shall be directly accessible without multi-step menu navigation.

## TR-UI-04 – No Visual Display Hardware
Derived From: FR-UI-05

The system shall not incorporate LCD, OLED, e-paper, or segmented numeric display modules.

## TR-UI-05 – Indicator Luminance Limit
Derived From: FR-M-05

Any mandatory status indicator shall:

Not exceed 0.1 lux at 0.5 m in a dark room
OR

Be automatically disabled during audio playback.

------------------------------------------------------------------------

# 10. Verification Conditions

Unless otherwise stated: - Ambient temperature: 20--25 °C\
- Background noise: ≤ 30 dBA\
- Measurement distance: 0.5 m for acoustic tests, 1.0 m for illuminance\
- Device warm-up time: 10 minutes before measurement

------------------------------------------------------------------------

**End of Document**
