# SleepCube – Functional Requirements Specification (FRS)

**Revision:** R00\
**Scope:** Defines what the product shall do. No implementation details.

---

# 1. Product Overview

SleepCube is a compact bedside device combining:

- Ambient sleep music playback  
- Dimmable night light  

The device shall provide a simple, calm, and predictable user experience intended to support falling asleep.

---

# 2. System Context

## 2.1 Operating Environment

FR-SC-01  
The device shall be intended for indoor residential bedroom use.

FR-SC-02  
The device shall operate on a stable flat surface such as a bedside table.

FR-SC-03  
The device shall be suitable for operation in low ambient light conditions.

---

# 3. Audio Function

## 3.1 Playback Capability

FR-A-01  
The device shall be capable of playing preloaded audio content.

FR-A-02  
The device shall automatically loop its available audio content.

FR-A-03  
The device shall not require user track selection.

FR-A-04  
Playback shall occur without audible glitches, interruptions, or unintended artifacts.

---

## 3.2 Playback Control

FR-A-05  
The user shall be able to start audio playback using a physical control.

FR-A-06  
The user shall be able to stop audio playback using a physical control.

FR-A-07  
Audio playback shall start without audible transients.

FR-A-08  
Audio playback shall stop without audible transients.

FR-A-09  
Audio level shall ramp smoothly when starting or stopping playback.

---

## 3.3 Volume Control

FR-A-10  
The device shall allow the user to increase playback volume.

FR-A-11  
The device shall allow the user to decrease playback volume.

FR-A-12  
The minimum volume level shall be barely audible in a quiet bedroom environment.

FR-A-13  
Volume changes shall occur smoothly without abrupt steps.

FR-A-14  
The device shall retain the last used volume level across normal power cycles.

FR-A-15  
The maximum volume level shall be limited to a level suitable for sleep use.

---

## 3.4 Sleep Timer

FR-A-16  
Audio playback shall always operate with an active sleep timer.

FR-A-17  
The sleep timer duration shall be predefined.

FR-A-18  
The user shall not be able to disable the sleep timer for continuous playback.

FR-A-19  
When the sleep timer expires, audio playback shall stop automatically.

FR-A-20  
Audio fade-out at timer expiration shall occur gradually and without audible artifacts.

FR-A-21  
The sleep timer function shall not affect the lighting function.

---

# 4. Lighting Function

## 4.1 Basic Operation

FR-L-01  
The device shall provide warm white illumination suitable for nighttime use.

FR-L-02  
The light output shall be visually comfortable in dark environments.

FR-L-03  
The user shall be able to turn the light on and off independently of audio playback.

---

## 4.2 Brightness Control

FR-L-04  
The device shall allow the user to adjust brightness.

FR-L-05  
Brightness changes shall appear smooth to the user.

FR-L-06  
The device shall support a very low brightness setting suitable for sleep environments.

FR-L-07  
The device shall retain the last used brightness level across normal power cycles.

FR-L-08  
The maximum brightness level shall be limited to a level suitable for sleep use.

---

## 4.3 Lighting Behavior

FR-L-09  
Light transitions (on/off and brightness changes) shall avoid abrupt visual disturbances.

FR-L-10  
Light fade-in and fade-out behavior shall be gradual.

---

# 5. User Interface

## 5.1 Controls

FR-UI-01  
The device shall use physical buttons for user interaction.

FR-UI-02  
The number of user-facing controls shall be minimal.

FR-UI-03  
The device shall provide controls for:
- Start/Stop audio  
- Volume adjustment  
- Light on/off  
- Brightness adjustment  

FR-UI-04  
The sleep timer shall activate automatically when audio playback starts.

FR-UI-05  
The device shall not require a display.

FR-UI-06  
The device shall not require wireless connectivity.

FR-UI-07  
The device shall not require a mobile application.

FR-UI-08  
Control behavior shall be consistent and predictable.

---

# 6. Power and Startup Behavior

FR-P-01  
The device shall operate from an external low-voltage DC power source.

FR-P-02  
The device shall power on in a defined default state.

FR-P-03  
The defined default startup state shall be:  
- Light active  
- Audio inactive  

FR-P-04  
After an uncontrolled power interruption, the device shall start in the defined default state.

FR-P-05  
The device shall not produce unintended sound or light during power-up.

FR-P-06  
Audio and lighting functions shall operate independently.

---

# 7. Mechanical & Physical Characteristics

FR-M-01  
The device shall have a cubic form factor.

FR-M-02  
Each side length shall be within approximately 50–100 mm.

FR-M-03  
The enclosure shall diffuse light to avoid direct glare.

FR-M-04  
The device shall be stable when placed on a flat surface.

FR-M-05  
The device shall not include bright status indicators visible during sleep use.

FR-M-06  
The device surface temperature shall remain safe and comfortable to touch during normal operation.

---

# 8. Audio Quality (User-Perceived)

FR-Q-01  
The audio output shall be free from audible hiss at typical sleep-use volume.

FR-Q-02  
The audio output shall be free from noticeable distortion at maximum intended sleep volume.

FR-Q-03  
The device shall not generate audible mechanical or electrical noise during operation.

---

**End of Document**

