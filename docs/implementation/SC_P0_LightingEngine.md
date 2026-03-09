# SleepCube P0 Lighting Engine and LED Streaming

## Purpose

Document the current warm-light rendering pipeline, animation layers, and RMT LED
streaming behavior for ESP32 DevKitC bring-up.

## Scope

- Included:
  - Addressable LED strip backend (RMT TX + custom WS2812 encoder)
  - Light service state model (enabled flag, target brightness, ramping)
  - Startup ramp with easing and overshoot
  - Ambient fluctuation (brightness + warmth)
  - Audio sway impulse on audio state transitions
  - Gamma mapping + temporal dithering for smoother fades
  - Non-blocking LED TX queueing with TX-done callback
- Excluded:
  - Multi-zone/per-pixel effect patterns
  - Color presets and user scene storage
  - DMA-backed LED streaming (not available on current classic ESP32 RMT path)

## Runtime Behavior

1. `sc_light_service_start()` initializes LED backend and starts `sc_light` task.
2. Task computes effective loop period from `SC_LIGHT_UPDATE_HZ` and RTOS tick.
3. Each loop:
   - ramps `current` brightness toward `target`
   - applies startup envelope
   - applies ambient fluctuation
   - applies optional audio sway envelope
   - renders warm RGB frame via light engine
   - queues frame for non-blocking RMT transmit
4. When disabled, service clears strip (subject to non-blocking TX availability).

## Rendering Model

`target_brightness` -> `current_brightness` (ramp) -> `startup_scale` ->
`+ fluctuation` -> `+ audio_sway` -> clamp [0..100] -> light engine:

- perceptual brightness (`gamma 2.2`)
- max brightness cap (`CONFIG_SC_LED_MAX_BRIGHTNESS_PCT`)
- warmth shift blend
- temporal dithering to reduce visible 8-bit stepping

## Interfaces

| API | Input | Output | Notes |
| --- | --- | --- | --- |
| `sc_light_service_start()` | none | `esp_err_t` | Initializes backend and task |
| `sc_light_service_set_enabled(enable)` | bool | `esp_err_t` | On/off state |
| `sc_light_service_toggle()` | none | `esp_err_t` | Toggle wrapper |
| `sc_light_service_change_brightness(step)` | signed step | `esp_err_t` | 5% step, min clamp at 5% |
| `sc_light_service_audio_sway(audio_enabled)` | bool | `esp_err_t` | Injects positive/negative sway envelope |
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
| `CONFIG_SC_LIGHT_DEFAULT_BRIGHTNESS_PCT` | `30` | Startup target |
| `CONFIG_SC_LIGHT_UPDATE_HZ` | `50` | Requested loop rate |
| `CONFIG_SC_LIGHT_RAMP_STEP_PCT` | `2` | Per-loop brightness ramp |
| `CONFIG_SC_LIGHT_STARTUP_RAMP_MS` | `3000` | Startup fade duration |
| `CONFIG_SC_LIGHT_STARTUP_OVERSHOOT_PCT` | `6` | Startup overshoot amount |
| `CONFIG_SC_LIGHT_FLUCT_ENABLE` | `y` | Ambient fluctuation switch |
| `CONFIG_SC_LIGHT_FLUCT_BRIGHTNESS_PCT` | `6` | Fluctuation amplitude (brightness) |
| `CONFIG_SC_LIGHT_FLUCT_WARMTH_PCT` | `10` | Fluctuation amplitude (warmth) |
| `CONFIG_SC_LIGHT_AUDIO_SWAY_ENABLE` | `y` | Audio sway switch |
| `CONFIG_SC_LIGHT_AUDIO_SWAY_PCT` | `8` | Audio sway amplitude |
| `CONFIG_SC_LIGHT_AUDIO_SWAY_MS` | `1400` | Audio sway duration |
| `CONFIG_SC_LIGHT_WARM_R/G/B` | `255/170/90` | Warm base color |

## Timing Notes

- Effective update rate is quantized by `CONFIG_FREERTOS_HZ`.
- Example: with `CONFIG_FREERTOS_HZ=100`, one tick is 10 ms, so practical loop rate
  is near 100 Hz max when using `vTaskDelayUntil()`.
- Light service clamps delay to at least one tick to avoid zero-tick assert.

## LED TX Strategy

- RMT configured with non-blocking queue mode.
- One shared TX buffer is reused.
- `s_tx_busy` is set on submit and cleared from RMT TX-done callback.
- New frames are skipped while TX is busy (`ESP_ERR_TIMEOUT`) to avoid buffer overwrite.

## Error Handling

- Init fails if RMT channel/encoder allocation fails.
- Frame writes return `ESP_ERR_INVALID_STATE` if backend not initialized.
- Frame writes/clear can return `ESP_ERR_TIMEOUT` when a previous transfer is still active.

## Constraints and Assumptions

- Current implementation renders a single uniform color for all LEDs.
- No per-frame retry queue in light service; skipped frames are allowed by design.
- Classic ESP32 RMT path here is non-DMA.

## Verification Notes

- Startup log shows loop timing, including target and effective Hz.
- Visual checks:
  - startup ramp duration follows `SC_LIGHT_STARTUP_RAMP_MS`
  - fluctuation is gentle and continuous
  - audio on/off triggers a short brightness sway
  - fades appear smoother due to gamma + dithering

## Open Items

- Move from step-based ramp to time-based slew (percent/second) for config-invariant feel.
- Add optional per-pixel gradients/pattern layers.
- Add instrumentation counters for dropped LED frames.
