# SleepCube P0 Runtime Services and UI Integration

## Purpose

Describe how board profile selection, service startup, event routing, and the current
LCD-touch interaction model are coordinated in the P0 firmware.

## Scope

- Included:
  - `app_core` event queue and dispatcher
  - service startup gating by `CONFIG_SC_ENABLE_*`
  - board/hardware profile logging
  - primary LCD-touch ambient UI on the Waveshare ESP32-C6 board
  - temporary GPIO button backend retained for bring-up/debug use
- Excluded:
  - gesture sets beyond tap, long-press, and direct slider drag
  - scene storage beyond brightness/volume persistence
  - power switch hardware integration

## Runtime Behavior

1. `app_main()` initializes settings storage and calls `sc_app_core_start()`.
2. `sc_hw_profile_log()` prints selected board profile and enabled service/backends.
3. `app_core` creates one event queue and one dispatcher task.
4. Services are started in order (audio, light, UI) when enabled in Kconfig.
5. Temporary button UI backends post typed events to the queue; `app_core` maps each event to service API calls.
6. The LCD touch UI posts audio-toggle events through `app_core`, while slider-mode drag adjustments call audio/light service setter APIs directly.

## Interfaces

| API | Input | Output | Notes |
| --- | --- | --- | --- |
| `sc_app_core_start()` | none | `esp_err_t` | Creates queue/task and starts enabled services |
| `sc_app_core_post_event(evt)` | event struct | `esp_err_t` | Non-blocking event push |
| `sc_hw_profile_log()` | none | `void` | Logs active board/service/backend config |
| `sc_ui_input_buttons_start()` | none | `esp_err_t` | Starts temporary GPIO button backend |
| `sc_lvgl_display_create_ambient_screen()` | none | `void` | Builds the ambient LCD control surface |
| `sc_audio_service_set_volume_percent(percent)` | 0-100 percent | `esp_err_t` | Direct absolute volume update used by LCD slider mode |
| `sc_light_service_set_brightness_percent(percent)` | 0-100 percent | `esp_err_t` | Direct absolute brightness update used by LCD slider mode |

## Event Mapping

| Event | Origin | Action |
| --- | --- | --- |
| `SC_APP_EVT_UI_AUDIO_TOGGLE` | rest-mode full-screen tap or temporary button backend | `sc_audio_service_toggle_playback()` |
| `SC_APP_EVT_UI_AUDIO_STOP` | reserved for future UX path | `sc_audio_service_set_playback(false)` |
| `SC_APP_EVT_UI_VOLUME_STEP` | temporary button backend | `sc_audio_service_change_volume(value)` |
| `SC_APP_EVT_UI_LIGHT_STEP` | temporary button backend | `sc_light_service_change_brightness(value)` |

## Configuration

| Key | Default | Effect |
| --- | --- | --- |
| `CONFIG_SC_BOARD_WAVESHARE_ESP32C6` | intended product path | Enables Waveshare touch-display profile |
| `CONFIG_SC_ENABLE_AUDIO` | `y` | Enables audio service startup |
| `CONFIG_SC_ENABLE_LIGHT` | `y` | Enables light service startup |
| `CONFIG_SC_ENABLE_UI` | `y` | Enables UI service startup |
| `CONFIG_SC_UI_BACKEND_LCD_TOUCH` | `y` on Waveshare | Enables primary LCD/touch backend |
| `CONFIG_SC_UI_INPUT_BUTTONS` | `n` on Waveshare, `y` on DevKitC | Keeps temporary button backend available for bring-up |

## LCD Touch Ambient UI

- `REST` mode is the default steady state.
- In `REST` mode the screen shows only a full-screen warm-white ambient field with no visible controls.
- A short tap anywhere in `REST` mode toggles audio playback through `app_core`.
- The ambient field reacts to tap toggles with a brief whole-screen brightness pulse for audio-on and a brief dimming pulse for audio-off.
- A hold of about 1 second enters `SLIDER` mode without toggling audio.
- `SLIDER` mode reveals two vertical drag controls:
  - left half: volume
  - right half: brightness
- Dragging upward increases the corresponding value.
- Slider input remains locked until the long-press finger is lifted once, preventing the press used to open `SLIDER` mode from also moving a control.
- `SLIDER` mode does not expose audio toggle.
- 3 seconds of inactivity return the UI from `SLIDER` mode to `REST` mode.
- Slider knobs use LVGL built-in symbol glyphs for quick visual identification:
  - volume: `LV_SYMBOL_VOLUME_MAX`
  - brightness: `LV_SYMBOL_EYE_OPEN`
- The primary UI exposes no separate light on/off control.

## LCD Ambient Rendering Model

- The LCD ambient renderer now keeps the screen pixels at a nearly static warm-white tone and animates the panel backlight PWM for the breathing effect.
- This avoids visible full-screen RGB565 stepping on the SPI-connected panel and produces smoother dark-room motion than per-frame full-screen color modulation.
- The right-hand slider updates both:
  - the dedicated-light target brightness through `sc_light_service_set_brightness_percent()`
  - the LCD ambient base level used by the backlight-driven renderer
- Audio on/off pulses are applied as additive backlight deltas on top of the filtered ambient level so they remain visible across a wider brightness range.
- The LCD ambient renderer still derives its steady-state behavior from shared audio/brightness service state, but it is not yet part of a shared LCD+LED animation engine.

## Temporary Button Backend

- Input mode: active-low with internal pull-up.
- Poll period: 20 ms.
- Edge behavior: action on press edge (`pressed && !prev_pressed`).
- Available debug functions: audio toggle, volume up/down, brightness up/down.

## Error Handling

- If a service is disabled at build-time, service API calls return `ESP_ERR_NOT_SUPPORTED` where applicable.
- If event queue creation or task creation fails, startup returns error.
- Queue full on event post returns `ESP_ERR_TIMEOUT`.

## Constraints and Assumptions

- The LCD UI uses a two-mode ambient-first interaction model rather than persistent visible controls.
- Temporary GPIO buttons remain non-primary and exist only for bring-up/debug flexibility.
- LCD and LED ambient behavior are coordinated through shared service state, but not yet through a single unified animation engine.

## Verification Notes

- Startup should include:
  - `sc_hw_profile: board profile: ...`
  - `sc_hw_profile: services: ...`
  - `sc_hw_profile: backends: ...`
- Boot should land in `REST` mode with ambient-only presentation.
- A short tap anywhere should toggle audio playback.
- A long press of about 1 second should reveal slider mode without toggling audio.
- Left slider drag should change volume.
- Right slider drag should change brightness.
- Slider mode should auto-hide after 3 seconds of inactivity.
- Long-press entry into slider mode should require a release and a new press before either slider can move.
- Ambient LCD breathing should remain visually smooth because the effect is driven primarily by backlight PWM rather than full-screen color repaints.

## Task Timing Diagram Generation

1. Enable `CONFIG_SC_TRACE_TIMING` in `menuconfig`.
2. Capture monitor output to file:
   - `idf.py -p <PORT> monitor | tee trace.log`
3. Generate markdown with Mermaid Gantt chart:
   - `python3 tools/generate_timing_diagram.py --input trace.log --output ../../docs/implementation/SC_P0_TaskTiming.md --window-ms 15000`
4. Open `docs/implementation/SC_P0_TaskTiming.md` in VS Code markdown preview.

## Open Items

- Improve LCD and LED strip synchronization beyond shared brightness/audio-state coupling.
- Add richer on-screen feedback without disrupting dark-room usability.
