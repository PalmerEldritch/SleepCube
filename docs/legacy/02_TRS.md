# SleepCube - Technical Requirements Specification (TRS)

**Revision:** R01\
**Derived from:** SleepCube FRS R01\
**Scope:** Defines technical requirements necessary to fulfill functional requirements.\
**Implementation:** Technology-neutral unless required for verification.

---

# 1. System Architecture Requirements

| ID | Derived From | Requirement |
| --- | --- | --- |
| TR-SC-01 | FR-A-01, FR-L-01, FR-UI-01, FR-P-01, FR-P-06 | The system shall be partitioned into at least the following functional subsystems: power, audio playback, dedicated lighting output, display/touch user interface, and control/logic. |
| TR-SC-02 | FR-L-03, FR-A-21, FR-P-06 | Audio and ambient-lighting subsystems shall be electrically and logically independent such that shutdown, timer expiry, or failure in the audio subsystem does not interrupt ambient visual presentation. |
| TR-SC-03 | FR-P-02, FR-P-04, FR-UI-06, FR-UI-07, FR-UI-08 | The control subsystem shall provide deterministic startup and runtime behavior without reliance on external communication, cloud services, or paired mobile software. |

---

# 2. Audio Requirements

| ID | Derived From | Requirement |
| --- | --- | --- |
| TR-A-01 | FR-A-15 | Maximum sound pressure level at 0.5 m shall be less than or equal to 55 dBA at maximum volume. |
| TR-A-02 | FR-A-12 | Minimum volume setting shall produce 25-30 dBA at 0.5 m. |
| TR-A-03 | FR-Q-01 | At typical sleep volume, A-weighted noise at 0.5 m with silent input shall be less than or equal to 20 dBA or greater than or equal to 50 dB SNR referenced to 40 dBA output. |
| TR-A-04 | FR-Q-02 | THD+N at 1 kHz into nominal load at maximum intended volume shall be less than or equal to 3 percent. |
| TR-A-05 | FR-A-07, FR-A-08 | Speaker terminal transient shall be less than or equal to 50 mV peak during start and stop events. |
| TR-A-06 | FR-A-09, FR-A-20 | Start fade-in shall complete within 0.3-1.0 s, manual stop fade-out shall complete within 0.3-1.0 s, and timer fade-out shall complete within 10-30 s. |
| TR-A-07 | FR-A-17 | Predefined sleep timer duration shall be 30 minutes. |
| TR-A-08 | FR-A-17, FR-A-19 | Timer accuracy shall be within plus or minus 2 percent or plus or minus 30 seconds, whichever is larger, over 0-40 C. |
| TR-A-09 | FR-A-19, FR-A-20 | Fade-out shall complete within 10-30 s after timer expiration. |
| TR-A-10 | FR-A-02 | Audio control logic shall automatically restart playback from the beginning of the available audio content within less than or equal to 100 ms of reaching end-of-content, without requiring user interaction. |
| TR-A-11 | FR-A-03 | The system shall expose no user interface function for selecting individual tracks. Audio content structure shall be treated as a single logical playback sequence. |
| TR-A-12 | FR-A-04 | Under steady-state operation, no buffer underruns shall occur at nominal input voltage range and no audible dropouts greater than 10 ms shall occur over 1 hour of continuous playback. |
| TR-A-13 | FR-A-10, FR-A-11 | The system shall provide at least 8 discrete volume levels across the defined SPL range, and adjacent volume steps shall not exceed 6 dB difference. |
| TR-A-14 | FR-A-13 | Volume adjustments shall ramp over 100-300 ms to prevent audible steps. |
| TR-A-15 | FR-A-16 | Audio playback state logic shall automatically initialize a countdown timer upon entering playback state, and no control path shall allow playback without an active timer instance. |
| TR-A-16 | FR-A-18 | Firmware shall not implement a continuous playback mode. Verification shall confirm the absence of a firmware state that enables indefinite playback. |
| TR-A-17 | FR-UI-04 | Timer initialization shall occur within less than or equal to 100 ms of entering audio playback state. |

---

# 3. Lighting Requirements

| ID | Derived From | Requirement |
| --- | --- | --- |
| TR-L-01 | FR-L-01 | The dedicated lighting output shall provide a warm-white appearance equivalent to 2200-2700 K. |
| TR-L-02 | FR-L-02, FR-L-08 | At 1.0 m distance, maximum dedicated-light brightness shall provide 10-30 lux. |
| TR-L-03 | FR-L-02, FR-L-06 | At 1.0 m distance, minimum dedicated-light brightness shall provide less than or equal to 0.3 lux. |
| TR-L-04 | FR-L-02, FR-L-09 | Percent flicker shall be less than or equal to 10 percent and flicker index shall be less than or equal to 0.1 at all dedicated-light brightness settings. |
| TR-L-05 | FR-L-05, FR-L-09, FR-L-10 | Brightness changes and coordinated ambient visual transitions shall ramp over 0.3-1.0 s. |
| TR-L-06 | FR-L-04 | The system shall provide at least 8 discrete brightness levels between minimum and maximum defined illuminance, and adjacent levels shall not exceed a factor of 1.8. |
| TR-L-07 | FR-L-03, FR-UI-05 | Display-based ambient visuals shall update to user-initiated brightness changes and audio-state changes within less than or equal to 100 ms and shall use a coordinated warm visual style with the dedicated lighting output. |

---

# 4. Display and User Interface Requirements

| ID | Derived From | Requirement |
| --- | --- | --- |
| TR-UI-01 | FR-A-05, FR-A-06, FR-UI-01 | The integrated touch interface shall detect and dispatch a user control action within less than or equal to 100 ms under normal operation. |
| TR-UI-02 | FR-UI-02 | The primary steady-state user interface shall expose no more than 3 interaction functions: audio playback toggle, volume adjustment, and brightness adjustment. |
| TR-UI-03 | FR-UI-03, FR-UI-08 | The primary touch interface shall provide direct access to audio playback toggle, volume increase/decrease, and brightness increase/decrease without multi-step menu navigation. |
| TR-UI-04 | FR-UI-01, FR-UI-05 | The system shall incorporate an integrated color display with local touch sensing and local rendering capability. |
| TR-UI-05 | FR-M-05, FR-UI-05 | Any non-ambient status indicator shall not exceed 0.1 lux at 0.5 m in a dark room or shall be automatically suppressed during sleep-use operation. |
| TR-UI-06 | FR-UI-09 | The primary user interface shall not expose a separate light on/off control. |

---

# 5. Power Requirements

| ID | Derived From | Requirement |
| --- | --- | --- |
| TR-P-01 | FR-P-01 | Nominal input voltage shall be 5.0 V DC with an allowable range of 4.75-5.25 V. |
| TR-P-02 | FR-SC-01, FR-SC-03 | Worst-case simultaneous operation shall not exceed 5 W input power. |
| TR-P-03 | FR-P-02, FR-P-03 | With ambient visual presentation active at minimum brightness and audio off, idle input power shall be less than or equal to 1.0 W average. |
| TR-P-04 | FR-A-15, FR-SC-01 | At typical sleep volume with minimum ambient brightness, input power shall be less than or equal to 1.5 W average. |
| TR-P-05 | FR-P-05 | During the first 200 ms after input voltage rises above 4.5 V, audio output shall remain muted and no visual output shall exceed 10 percent of maximum ambient brightness before control-subsystem initialization completes. |

---

# 6. Thermal and Mechanical Requirements

| ID | Derived From | Requirement |
| --- | --- | --- |
| TR-M-01 | FR-M-06 | Accessible surfaces shall not exceed 42 C at 25 C ambient after 4 hours steady-state or 48 C at 35 C ambient after 4 hours steady-state. |
| TR-M-02 | FR-M-06 | Local hotspots shall not exceed the applicable surface limit by more than 3 C. |
| TR-M-03 | FR-Q-03 | With audio muted and ambient visual presentation active at any brightness, device acoustic noise at 0.5 m shall be less than or equal to 20 dBA. |
| TR-M-04 | FR-SC-02, FR-M-04 | The device shall remain stable and not tip when placed on a horizontal surface tilted up to 10 degrees in any direction and subjected to a 2 N horizontal force applied at the top edge. |
| TR-M-05 | FR-M-01 | All three external side lengths shall be equal within plus or minus 2 mm tolerance. |
| TR-M-06 | FR-M-02 | External side length shall be between 50 mm and 100 mm inclusive. |
| TR-M-07 | FR-M-03 | No directly visible LED die shall be observable from any normal viewing angle at 0.5 m distance, and maximum luminance at the enclosure surface shall not exceed 5000 cd/m2. |

---

# 7. Non-Volatile Memory Requirements

| ID | Derived From | Requirement |
| --- | --- | --- |
| TR-DS-04 | FR-A-14, FR-L-07 | Stored user parameters shall retain data for at least 5 years without power. |
| TR-DS-05 | FR-A-14, FR-L-07 | Settings storage shall support at least 50,000 update cycles. |

---

# 8. Verification Conditions

| ID | Condition |
| --- | --- |
| VC-01 | Ambient temperature shall be 20-25 C unless otherwise stated. |
| VC-02 | Background noise shall be less than or equal to 30 dBA for acoustic tests. |
| VC-03 | Measurement distance shall be 0.5 m for acoustic tests and 1.0 m for illuminance tests unless otherwise stated. |
| VC-04 | Device warm-up time shall be 10 minutes before measurement unless otherwise stated. |

---

**End of Document**
