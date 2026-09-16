# SleepCube P0 Audio Playback Implementation

## Purpose

Provide the current P0 audio playback baseline for the Waveshare ESP32-C6 touch board.
The firmware prefers MP3 data from the SD card, falls back to onboard flash (SPIFFS),
decodes to PCM, and transmits PCM over I2S to an external mono amplifier/speaker path.

## Scope

- Included:
  - audio control service (`start`, `toggle`, `set playback`, `volume step`)
  - mandatory 30-minute sleep timer started on playback enable
  - manual stop fade-out and timer-expiry fade-out
  - persisted volume restore through NVS
  - SPIFFS mount of partition label `storage`
  - SD card mount on `/sdcard` for the Waveshare ESP32-C6 touch board
  - preferred MP3 playback from `/sdcard/test.mp3` with SPIFFS fallback to `/spiffs/test.mp3`
  - runtime serial diagnostics console for development builds
  - runtime SD track selection from the card root
  - runtime diagnostic tone/sweep source selection
  - runtime MP3/WAV selection by chosen file extension
  - minimal PCM WAV playback path for comparison testing
  - Helix MP3 decode to 16-bit PCM
  - live software volume scaling during playback
  - optional runtime MP3 pre-gain, mix mode, limiter, and EQ diagnostics
  - I2S TX output at fixed 44.1 kHz using `MSB` framing and `32-bit` slot width
  - board-specific I2S pin mapping for the Waveshare ESP32-C6 touch display board
  - explicit TX enable/disable on playback start/stop to force silent stop behavior
  - shared-bus arbitration between SD reads and LCD refresh on `SPI2`
  - optional I2S RX loopback monitor on boards that expose a usable RX path
- Excluded:
  - resampling for non-44.1 kHz MP3 content
  - track selection or playlist management
  - configurable timer duration

## Runtime Behavior

1. `sc_audio_service_start()` restores persisted volume, starts the subsystem, and leaves playback disabled by default.
2. `sc_audio_player_start()` mounts SPIFFS, attempts to mount the SD card, optionally starts loopback monitoring, initializes I2S TX at 44.1 kHz, and creates the playback task.
3. When playback is enabled:
   - a fixed sleep timer is started
   - TX is enabled
   - the selected playback path is resolved
   - MP3 content is decoded and written to I2S, or PCM WAV is streamed directly
4. Volume changes update the active playback path immediately through per-frame software scaling.
5. Manual stop requests initiate a short fade-out before playback is disabled.
6. Timer expiry initiates a long fade-out before playback is disabled.
7. On transition back to disabled, TX is disabled so the amplifier path goes silent without residual output.

## Interfaces

| API | Input | Output | Notes |
| --- | --- | --- | --- |
| `sc_audio_service_start()` | none | `esp_err_t` | Restores volume, starts service |
| `sc_audio_service_set_playback(enable)` | bool | `esp_err_t` | Starts playback or requests fade-out stop |
| `sc_audio_service_toggle_playback()` | none | `esp_err_t` | Toggle playback state |
| `sc_audio_service_change_volume(delta_steps)` | signed step | `esp_err_t` | Volume step is 5 percent per step |
| `sc_audio_player_start()` | none | `esp_err_t` | Subsystem startup orchestration |
| `sc_audio_player_set_enabled(enable)` | bool | none | Starts or hard-stops playback state |
| `sc_audio_player_request_stop()` | none | none | Requests manual stop fade-out |
| `sc_audio_player_get_effective_volume_percent()` | none | `uint8_t` | Current runtime gain after fade/timer logic |
| `sc_audio_fs_mount()` | none | `esp_err_t` | Mounts SPIFFS and attempts SD mount |
| `sc_audio_fs_get_default_mp3_path()` | none | `const char *` | Resolves preferred startup path |
| `sc_audio_mp3_play_file(path, play_enabled)` | MP3 path + playback state pointer | `esp_err_t` | Decode/playback loop body |
| `sc_audio_wav_play_file(path, play_enabled)` | WAV path + playback state pointer | `esp_err_t` | PCM WAV playback loop body |
| `sc_audio_i2s_init(rate)` | sample rate | `esp_err_t` | TX init |
| `sc_audio_i2s_set_tx_enabled(enable)` | bool | `esp_err_t` | Explicit TX gate for start/stop silence |
| `sc_audio_i2s_write(buf, frames, timeout)` | interleaved stereo PCM | `esp_err_t` | Frame write to TX |

## Configuration

| Key | Default | Effect |
| --- | --- | --- |
| `CONFIG_SC_ENABLE_AUDIO` | `y` | Builds and starts audio service |
| `CONFIG_SC_LOOPBACK_ENABLE` | board-dependent | Enables loopback task on supported boards |
| `Partition table storage` | `0x50000` | Space for SPIFFS audio files |
| `Preferred playback path` | `/sdcard/test.mp3` | Used when SD is mounted and file exists |
| `Fallback playback path` | `/spiffs/test.mp3` | Used when SD is unavailable or file is absent |
| `Track selection` | runtime serial CLI | Development diagnostics can select SD-root `.mp3` and `.wav` files |
| `Sleep timer duration` | fixed 30 minutes | Started automatically with playback |
| `Manual stop fade` | fixed 0.5 s | Used for user stop requests |
| `Timer fade` | fixed 15 s | Used on timer expiry |

## Data Flow

Preferred path:
`SD file` -> `LVGL/display bus lock` -> `fread()` chunks -> `unlock` -> `Helix MP3 decoder` -> `software volume/fade scaling` -> `I2S TX`

Fallback path:
`SPIFFS file` -> `fread()` chunks -> `Helix MP3 decoder` -> `software volume/fade scaling` -> `I2S TX`

WAV comparison path:
`SD or SPIFFS WAV file` -> `LVGL/display bus lock` for SD reads when needed -> `PCM 16-bit frames` -> `software volume scaling` -> `I2S TX`

## Board Integration

### Waveshare ESP32-C6 Touch Display Board

| Signal | GPIO | Note |
| --- | --- | --- |
| `WS / LRCLK` | `GPIO5` | Audio frame sync |
| `BCLK` | `GPIO6` | Audio bit clock |
| `DOUT` | `GPIO7` | Serial PCM output |
| `AMP SD` | not used in firmware | Shutdown tied high in hardware |
| `SD CS` | `GPIO4` | SD card chip select on shared SPI host |

## Error Handling

- SPIFFS mount errors stop startup and return `esp_err_t`.
- SD mount failure is logged and does not stop startup; playback falls back to SPIFFS.
- MP3 file open failure logs and returns `ESP_ERR_NOT_FOUND`.
- Decode buffer allocation failure returns `ESP_ERR_NO_MEM`.
- I2S write/read errors are logged and surfaced as `esp_err_t`.
- Settings load/save failures are logged and fall back to defaults or best-effort operation.
- If the LVGL bus lock cannot be acquired before an SD read, playback fails rather than risking shared-bus corruption.

## Constraints and Assumptions

- Playback rate is fixed at 44.1 kHz; non-44.1 kHz MP3 plays without resampling.
- Startup auto-path still targets a fixed default file, but development diagnostics can override the selected asset at runtime.
- Timer duration is fixed in firmware.
- The current amplifier hardware is mono; stereo input is preserved on the bus but may be summed in the external amplifier stage.
- SD and LCD share the same SPI host and rely on software serialization for coexistence.
- WAV support is intentionally limited to PCM, 16-bit, mono/stereo content.

## Verification Notes

- Verified functional behavior:
  - clean MP3 playback from `/sdcard/test.mp3`
  - SPIFFS fallback remains available if the SD file is absent
  - live volume changes affect active playback
  - manual stop fades out before mute
  - timer expiry fades out before mute

## Open Items

- Add explicit fade-in path matching the stop/timer fade behavior.
- Add resampling or dynamic I2S reconfiguration for sample-rate mismatch.
- Add content-selection strategy if the product moves beyond a fixed logical sequence.
- See `SC_P0_AudioInvestigation.md` for current diagnostic findings and hardware-dependent open questions.
