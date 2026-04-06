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
  - final gesture-rich UI polish
  - scene storage beyond brightness/volume persistence
  - power switch hardware integration

## Runtime Behavior

1. `app_main()` initializes settings storage and calls `sc_app_core_start()`.
2. `sc_hw_profile_log()` prints selected board profile and enabled service/backends.
3. `app_core` creates one event queue and one dispatcher task.
4. Services are started in order (audio, light, UI) when enabled in Kconfig.
5. UI backends post typed events to the queue; `app_core` maps each event to service API calls.

## Interfaces

| API | Input | Output | Notes |
| --- | --- | --- | --- |
| `sc_app_core_start()` | none | `esp_err_t` | Creates queue/task and starts enabled services |
| `sc_app_core_post_event(evt)` | event struct | `esp_err_t` | Non-blocking event push |
| `sc_hw_profile_log()` | none | `void` | Logs active board/service/backend config |
| `sc_ui_input_buttons_start()` | none | `esp_err_t` | Starts temporary GPIO button backend |
| `sc_lvgl_display_create_ambient_screen()` | none | `void` | Builds the ambient LCD control surface |

## Event Mapping

| Event | Origin | Action |
| --- | --- | --- |
| `SC_APP_EVT_UI_AUDIO_TOGGLE` | touch center zone or temporary button backend | `sc_audio_service_toggle_playback()` |
| `SC_APP_EVT_UI_AUDIO_STOP` | reserved for future UX path | `sc_audio_service_set_playback(false)` |
| `SC_APP_EVT_UI_VOLUME_STEP` | touch edge zones or temporary button backend | `sc_audio_service_change_volume(value)` |
| `SC_APP_EVT_UI_LIGHT_STEP` | touch edge zones or temporary button backend | `sc_light_service_change_brightness(value)` |

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

- Center touch zone toggles audio playback.
- Upper and lower right zones step volume up and down.
- Upper and lower left zones step brightness up and down.
- The display background renders a warm ambient field that reacts to current brightness and audio state.
- The primary UI exposes no separate light on/off control.

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

- The current LCD UI uses static touch zones rather than a fully polished gesture model.
- Temporary GPIO buttons remain non-primary and exist only for bring-up/debug flexibility.
- LCD and LED ambient behavior are coordinated through shared service state, but not yet through a single unified animation engine.

## Verification Notes

- Startup should include:
  - `sc_hw_profile: board profile: ...`
  - `sc_hw_profile: services: ...`
  - `sc_hw_profile: backends: ...`
- Touching the center zone should toggle audio playback.
- Touching left-side zones should change brightness.
- Touching right-side zones should change volume.

## Task Timing Diagram Generation

1. Enable `CONFIG_SC_TRACE_TIMING` in `menuconfig`.
2. Capture monitor output to file:
   - `idf.py -p <PORT> monitor | tee trace.log`
3. Generate markdown with Mermaid Gantt chart:
   - `python3 tools/generate_timing_diagram.py --input trace.log --output ../../docs/implementation/SC_P0_TaskTiming.md --window-ms 15000`
4. Open `docs/implementation/SC_P0_TaskTiming.md` in VS Code markdown preview.

## Open Items

- Replace static touch zones with a more polished final interaction design.
- Improve LCD and LED strip synchronization beyond shared brightness/audio-state coupling.
- Add richer on-screen feedback without disrupting dark-room usability.
