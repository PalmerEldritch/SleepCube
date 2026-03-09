# SleepCube P0 Runtime Services and UI Integration

## Purpose

Describe how board profile selection, service startup, and UI events are coordinated
in the current PoC firmware.

## Scope

- Included:
  - `app_core` event queue and dispatcher
  - Service startup gating by `CONFIG_SC_ENABLE_*`
  - Board/hardware profile logging
  - Temporary GPIO button input backend used before LCD-touch hardware arrives
- Excluded:
  - Final LCD touch gesture implementation
  - Persistent settings storage
  - Power switch hardware integration

## Runtime Behavior

1. `app_main()` calls `sc_app_core_start()`.
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

## Event Mapping

| Event | Origin | Action |
| --- | --- | --- |
| `SC_APP_EVT_UI_AUDIO_TOGGLE` | button or future touch backend | `sc_audio_service_toggle_playback()` |
| `SC_APP_EVT_UI_AUDIO_STOP` | future touch double-tap backend | `sc_audio_service_set_playback(false)` |
| `SC_APP_EVT_UI_VOLUME_STEP` | button or future touch slider | `sc_audio_service_change_volume(value)` |
| `SC_APP_EVT_UI_LIGHT_STEP` | button or future touch slider | `sc_light_service_change_brightness(value)` |
| `SC_APP_EVT_UI_LIGHT_TOGGLE` | temporary button backend | `sc_light_service_toggle()` |

## Configuration

| Key | Default | Effect |
| --- | --- | --- |
| `CONFIG_SC_BOARD_DEVKITC_ESP32` | `y` | Current bring-up board profile |
| `CONFIG_SC_ENABLE_AUDIO` | `y` | Enables audio service startup and real implementation |
| `CONFIG_SC_ENABLE_LIGHT` | `y` | Enables light service startup |
| `CONFIG_SC_ENABLE_UI` | `y` | Enables UI service startup |
| `CONFIG_SC_UI_INPUT_BUTTONS` | `y` on DevKitC | Enables temporary button backend |
| `CONFIG_SC_UI_BACKEND_LCD_TOUCH` | `n` on DevKitC | Reserved for Waveshare ESP32-C6 profile |

## Temporary Button Backend

- Input mode: active-low with internal pull-up.
- Poll period: 20 ms.
- Edge behavior: action on press edge (`pressed && !prev_pressed`).
- Default GPIO map is documented in `board_pins.h` and configurable in `menuconfig`.

## Error Handling

- If a service is disabled at build-time, service API calls return `ESP_ERR_NOT_SUPPORTED`
  where applicable.
- If event queue creation or task creation fails, startup returns error.
- Queue full on event post returns `ESP_ERR_TIMEOUT`.

## Constraints and Assumptions

- No debounce/filtering beyond 20 ms polling and edge-detection.
- `SC_APP_EVT_UI_LIGHT_TOGGLE` exists for temporary button testing and may be removed
  when final display UX is active.
- UI service currently supports "buttons only" on DevKitC and logs LCD backend as inactive.

## Verification Notes

- Startup should include:
  - `sc_hw_profile: board profile: ...`
  - `sc_hw_profile: services: ...`
  - `sc_hw_profile: backends: ...`
  - `sc_ui_buttons: GPIO map ...`
- Button presses should log one action per press edge and produce corresponding service log.

## Task Timing Diagram Generation

1. Enable `CONFIG_SC_TRACE_TIMING` in `menuconfig`.
2. Capture monitor output to file (example):
   - `idf.py -p <PORT> monitor | tee trace.log`
3. Generate markdown with Mermaid Gantt chart:
   - `python3 tools/generate_timing_diagram.py --input trace.log --output ../../docs/implementation/SC_P0_TaskTiming.md --window-ms 15000`
4. Open `docs/implementation/SC_P0_TaskTiming.md` in VS Code markdown preview.

Notes:
- Trace markers are emitted from `app_core`, `audio`, `light`, and `ui_btn`.
- Light task markers are decimated (`1/5` cycles) to keep logs manageable.
- Diagram quality improves when capture focuses on one scenario (for example: boot -> start audio -> adjust light -> stop audio).

## Open Items

- Implement LCD-touch backend and map gestures to same app events.
- Remove temporary light toggle button path if final UX keeps light tied to power switch.
- Add lightweight debounce strategy if needed for external mechanical buttons.
