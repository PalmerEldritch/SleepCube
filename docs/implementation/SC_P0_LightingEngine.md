# SleepCube P0 Lighting Engine and LED Streaming

## Purpose

Document the current dedicated-light rendering pipeline, ambient animation layers,
and LCD synchronization inputs used in the P0 firmware.

## Scope

- Included:
  - addressable LED strip backend (RMT TX + custom WS2812 encoder)
  - light service state model (enabled flag, target brightness, ramping)
  - persisted brightness restore at startup
  - startup ramp with easing and overshoot
  - global ambient fluctuation (brightness + warmth)
  - spatial undulation across the LED strip
  - audio sway impulse on audio state transitions
  - gamma mapping + temporal dithering for smoother fades
  - LCD ambient renderer consumption of current brightness/audio state
  - LCD backlight-driven breathing and pulse rendering used by the touch display UI
  - non-blocking LED TX queueing with TX-done callback
- Excluded:
  - multi-scene storage
  - final shared animation engine between LCD and strip
  - DMA-backed LED streaming

## Runtime Behavior

1. `sc_light_service_start()` restores persisted brightness, initializes LED backend, and starts `sc_light` task.
2. Task computes effective loop period from `SC_LIGHT_UPDATE_HZ` and RTOS tick.
3. Each loop:
   - ramps `current` brightness toward `target`
   - applies startup envelope
   - applies ambient fluctuation
   - applies optional audio sway envelope
   - renders warm RGB frame via light engine
   - queues frame for non-blocking RMT transmit
4. The LCD ambient layer reads target brightness and audio state from services to stay visually aligned with the dedicated light output.
5. The LCD ambient layer keeps a stable warm-white pixel field and modulates panel backlight PWM for breathing and audio-toggle pulse response.

## Rendering Model

`target_brightness` -> `current_brightness` -> `startup_scale` -> `+ fluctuation` -> `+ audio_sway` -> clamp [0..100] -> light engine:

- perceptual brightness (`gamma 2.2`)
- max brightness cap (`CONFIG_SC_LED_MAX_BRIGHTNESS_PCT`)
- warmth shift blend
- spatial undulation along the strip
- temporal dithering to reduce visible 8-bit stepping

## Interfaces

| API | Input | Output | Notes |
| --- | --- | --- | --- |
| `sc_light_service_start()` | none | `esp_err_t` | Restores brightness, initializes backend, starts task |
| `sc_light_service_set_enabled(enable)` | bool | `esp_err_t` | Internal/service-level on/off state |
| `sc_light_service_toggle()` | none | `esp_err_t` | Legacy wrapper, not part of primary UI |
| `sc_light_service_change_brightness(step)` | signed step | `esp_err_t` | 5 percent step, min clamp at 5 percent |
| `sc_light_service_audio_sway(audio_enabled)` | bool | `esp_err_t` | Injects positive/negative sway envelope |
| `sc_light_service_get_current_brightness_percent()` | none | `uint8_t` | Exposes current brightness for LCD sync |
| `sc_light_service_get_target_brightness_percent()` | none | `uint8_t` | Exposes target brightness |
| `sc_lcd_panel_if_set_backlight_level(level)` | 0-1023 duty | `esp_err_t` | Used by LCD ambient renderer for smooth PWM-driven breathing and pulse output |
| `sc_led_strip_if_write_rgb(buf, count)` | RGB bytes | `esp_err_t` | Non-blocking TX submit |
| `sc_light_engine_render_warm(...)` | brightness/warmth/frame | none | Fills RGB frame buffer |

## Configuration

| Key | Default | Effect |
| --- | --- | --- |
| `CONFIG_SC_LIGHT_BACKEND_LED_STRIP` | `y` | Enables LED strip backend |
| `CONFIG_SC_LED_STRIP_GPIO` | `13` | Data output pin |
| `CONFIG_SC_LED_COUNT` | `28` | Number of LEDs |
| `CONFIG_SC_LED_PIXEL_ORDER_*` | `GRB` | Channel order for strip |
| `CONFIG_SC_LED_MAX_BRIGHTNESS_PCT` | `60` | Global cap |
| `CONFIG_SC_LIGHT_DEFAULT_BRIGHTNESS_PCT` | `30` | Fallback startup target |
| `CONFIG_SC_LIGHT_UPDATE_HZ` | `50` | Requested loop rate |
| `CONFIG_SC_LIGHT_RAMP_STEP_PCT` | `2` | Per-loop brightness ramp |
| `CONFIG_SC_LIGHT_STARTUP_RAMP_MS` | `3000` | Startup fade duration |
| `CONFIG_SC_LIGHT_STARTUP_OVERSHOOT_PCT` | `6` | Startup overshoot amount |
| `CONFIG_SC_LIGHT_FLUCT_ENABLE` | `y` | Ambient fluctuation switch |
| `CONFIG_SC_LIGHT_FLUCT_BRIGHTNESS_PCT` | `6` | Fluctuation amplitude (brightness) |
| `CONFIG_SC_LIGHT_FLUCT_WARMTH_PCT` | `10` | Fluctuation amplitude (warmth) |
| `CONFIG_SC_LIGHT_SPATIAL_UNDULATION_ENABLE` | `y` | Enables strip-wide spatial movement |
| `CONFIG_SC_LIGHT_SPATIAL_UNDULATION_PCT` | `12` | Spatial amplitude |
| `CONFIG_SC_LIGHT_SPATIAL_KNOTS` | `4` | Spatial control points |
| `CONFIG_SC_LIGHT_SPATIAL_SPEED_PCT` | `70` | Spatial motion speed |
| `CONFIG_SC_LIGHT_AUDIO_SWAY_ENABLE` | `y` | Audio sway switch |
| `CONFIG_SC_LIGHT_AUDIO_SWAY_PCT` | `8` | Audio sway amplitude |
| `CONFIG_SC_LIGHT_AUDIO_SWAY_MS` | `1400` | Audio sway duration |
| `CONFIG_SC_LIGHT_WARM_R/G/B` | `255/170/90` | Warm base color |

## LED TX Strategy

- RMT is configured with non-blocking queue mode.
- One shared TX buffer is reused.
- `s_tx_busy` is set on submit and cleared from the RMT TX-done callback.
- New frames are skipped while TX is busy to avoid buffer overwrite.

## Error Handling

- Init fails if RMT channel or encoder allocation fails.
- Frame writes return `ESP_ERR_INVALID_STATE` if backend is not initialized.
- Frame writes and clear can return `ESP_ERR_TIMEOUT` when a previous transfer is still active.
- Brightness persistence falls back to default if settings load fails.

## Constraints and Assumptions

- LCD and strip are not yet driven by a single shared animation-state object.
- Display synchronization currently uses shared service state rather than per-frame strip data.
- LCD ambient brightness changes are expressed primarily through backlight PWM because repeated full-screen RGB565 brightness updates on the SPI panel produced visible stepping.
- No per-frame retry queue exists in light service; skipped frames are allowed by design.

## Verification Notes

- Startup log shows loop timing, including target and effective Hz.
- Visual checks:
  - startup ramp duration follows `SC_LIGHT_STARTUP_RAMP_MS`
  - fluctuation is gentle and continuous
  - spatial movement remains calm rather than attention-grabbing
  - audio on/off triggers a short brightness sway
  - LCD ambience tracks brightness/audio state changes
  - LCD breathing remains visually smooth and audio-toggle pulses remain visible across the practical brightness range

## Open Items

- Move from step-based ramp to time-based slew for config-invariant feel.
- Replace loose LCD-state coupling with a shared visual-state model.
- Add instrumentation counters for dropped LED frames.
