# Implementation Documentation Queue

Use this table to promote code areas from "working" to "documented and stable".

| Item | Scope | Status | Trigger | Owner | Notes |
| --- | --- | --- | --- | --- | --- |
| `sc_audio_fs_mount()` | SPIFFS mount, SD mount, and playback-path resolution | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_i2s_*` | I2S TX/RX init, TX gating, and frame IO | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` for current Waveshare ESP32-C6 audio format and pin mapping |
| `sc_audio_mp3_play_file()` | MP3 decode, PCM output, and shared-bus-safe SD reads | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_wav_play_file()` | PCM WAV playback for comparison diagnostics | Documented | implementation update | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_loopback_start()` | Digital signal verification | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_player_start()` | Startup orchestration and playback task baseline | Documented | `@docready` in header | Firmware | See `SC_P0_AudioPlayback.md` |
| `sc_audio_service_*` | Audio control API (playback, timer, volume persistence) | Documented | `promote: audio-control` | Firmware | See `SC_P0_AudioPlayback.md`; volume is restored from NVS and playback always starts a timer |
| `Audio diagnostics investigation` | Runtime console experiments, SD bring-up findings, and codec/hardware observations | Working Notes | ongoing hardware investigation | Firmware | See `SC_P0_AudioInvestigation.md`; pause further tuning until replacement driver is available |
| `sc_light_service_*` | Light state, persistence, and animation orchestration | Documented | `promote: light-engine` | Firmware | See `SC_P0_LightingEngine.md`; brightness is restored from NVS |
| `sc_light_engine_render_warm()` | Warm render, gamma mapping, temporal dithering | Documented | `promote: light-engine` | Firmware | See `SC_P0_LightingEngine.md` |
| `sc_led_strip_if_*` | Non-blocking RMT LED streaming backend | Documented | `promote: light-engine` | Firmware | See `SC_P0_LightingEngine.md` |
| `sc_app_core_*` | Service startup + event dispatch | Documented | `promote: runtime-core` | Firmware | See `SC_P0_RuntimeServices.md`; primary UI model is audio toggle + volume + brightness |
| `sc_ui_input_buttons_*` | Temporary GPIO button backend | Documented | `promote: ui-buttons` | Firmware | See `SC_P0_RuntimeServices.md` |
| `sc_hw_profile_log()` | Build-profile/config visibility at boot | Documented | `promote: runtime-core` | Firmware | See `SC_P0_RuntimeServices.md` |
| `sc_trace_mark()` + `generate_timing_diagram.py` | Runtime timing trace and generated schedule diagram | Documented | `promote: task-timing` | Firmware | See `SC_P0_RuntimeServices.md` and `SC_P0_TaskTiming.md` |
