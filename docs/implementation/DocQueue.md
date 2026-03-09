# Implementation Documentation Queue

Use this table to promote code areas from "working" to "documented and stable".

| Item | Scope | Status | Trigger | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| `sc_audio_fs_mount()` | SPIFFS mount and partition assumptions | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_i2s_*` | I2S TX/RX init and frame IO | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_mp3_play_file()` | MP3 decode and PCM output | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_loopback_start()` | Digital signal verification | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_player_start()` | Startup orchestration | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_service_*` | Audio control API (playback + volume) | Documented | `promote: audio-control` | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_light_service_*` | Light state and animation orchestration | Documented | `promote: light-engine` | Firmware | See `SC_P0_LightingEngine.md` |
| `sc_light_engine_render_warm()` | Warm render, gamma mapping, temporal dithering | Documented | `promote: light-engine` | Firmware | See `SC_P0_LightingEngine.md` |
| `sc_led_strip_if_*` | Non-blocking RMT LED streaming backend | Documented | `promote: light-engine` | Firmware | See `SC_P0_LightingEngine.md` |
| `sc_app_core_*` | Service startup + event dispatch | Documented | `promote: runtime-core` | Firmware | See `SC_P0_RuntimeServices.md` |
| `sc_ui_input_buttons_*` | Temporary GPIO button backend | Documented | `promote: ui-buttons` | Firmware | See `SC_P0_RuntimeServices.md` |
| `sc_hw_profile_log()` | Build-profile/config visibility at boot | Documented | `promote: runtime-core` | Firmware | See `SC_P0_RuntimeServices.md` |
| `sc_trace_mark()` + `generate_timing_diagram.py` | Runtime timing trace and generated schedule diagram | Documented | `promote: task-timing` | Firmware | See `SC_P0_RuntimeServices.md` and `SC_P0_TaskTiming.md` |
