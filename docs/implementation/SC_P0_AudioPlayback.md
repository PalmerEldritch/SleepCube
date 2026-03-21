# SleepCube P0 Audio Playback Implementation

## Purpose

Provide the current P0 audio playback baseline for the Waveshare ESP32-C6 touch board.
The firmware reads MP3 data from onboard flash (SPIFFS), decodes to PCM, and transmits
PCM over I2S to an external mono amplifier/speaker path.

## Scope

- Included:
  - Audio control service (`start`, `toggle`, `set playback`, `volume step`)
  - SPIFFS mount of partition label `storage`
  - MP3 file playback from `/spiffs/test.mp3`
  - Helix MP3 decode to 16-bit PCM
  - I2S TX output at fixed 44.1 kHz using `MSB` framing and `32-bit` slot width
  - Board-specific I2S pin mapping for the Waveshare ESP32-C6 touch display board
  - Explicit TX enable/disable on playback start/stop to force silent stop behavior
  - Volume control in 5% steps
  - Optional I2S RX loopback monitor (`CONFIG_SC_LOOPBACK_ENABLE`) on boards that expose a usable RX path
- Excluded:
  - Resampling for non-44.1 kHz MP3 content
  - Track selection / playlist management
  - SD card file playback
  - Playlist / media library management

## Runtime Behavior

1. `sc_audio_service_start()` initializes the subsystem and sets defaults (`enabled=0`, `volume=40%`).
2. `sc_audio_player_start()` mounts SPIFFS, optionally starts loopback monitoring, initializes I2S TX at 44.1 kHz, and creates the playback task.
3. Audio task runs continuously:
   - When disabled: TX is disabled and the task sleeps.
   - When enabled: TX is enabled, `/spiffs/test.mp3` is decoded, scaled by current volume, and written to I2S.
4. End-of-file or stop request exits decode loop.
5. On transition back to disabled, TX is disabled again so the amplifier path goes silent without residual output.

## Interfaces

| API | Input | Output | Notes |
| --- | --- | --- | --- |
| `sc_audio_service_start()` | none | `esp_err_t` | Starts service and sets defaults |
| `sc_audio_service_set_playback(enable)` | bool | `esp_err_t` | Explicit playback control |
| `sc_audio_service_toggle_playback()` | none | `esp_err_t` | Toggle playback state |
| `sc_audio_service_change_volume(delta_steps)` | signed step | `esp_err_t` | Volume step is 5% per step |
| `sc_audio_player_start()` | none | `esp_err_t` | Subsystem startup orchestration |
| `sc_audio_fs_mount()` | none | `esp_err_t` | Mounts SPIFFS partition `storage` |
| `sc_audio_mp3_play_file(path, play_enabled, volume)` | MP3 path + playback state pointer + volume | `esp_err_t` | Decode and playback loop body |
| `sc_audio_i2s_init(rate)` | sample rate | `esp_err_t` | TX init |
| `sc_audio_i2s_set_tx_enabled(enable)` | bool | `esp_err_t` | Explicit TX gate for start/stop silence |
| `sc_audio_i2s_write(buf, frames, timeout)` | interleaved stereo PCM | `esp_err_t` | Frame write to TX |
| `sc_audio_i2s_init_rx(rate)` | sample rate | `esp_err_t` | RX init for loopback |
| `sc_audio_i2s_read(buf, frames, out, timeout)` | RX frame request | `esp_err_t` | Frame read from RX |
| `sc_audio_loopback_start(rate)` | sample rate | `esp_err_t` | Starts verification task |

## Configuration

| Key | Default | Effect |
| --- | --- | --- |
| `CONFIG_SC_ENABLE_AUDIO` | `y` in current bring-up | Builds and starts audio service |
| `CONFIG_SC_LOOPBACK_ENABLE` | board-dependent | Enables loopback task and RX channel init where supported |
| Partition table `storage` | 0x50000 | Space for SPIFFS audio files |
| I2S sample rate | 44100 Hz | TX and loopback RX operating rate |
| Playback file path | `/spiffs/test.mp3` | Fixed MP3 source for current bring-up |

## Data Flow

`SPIFFS file` -> `fread()` chunks -> `Helix MP3 decoder` -> `PCM frames` -> `I2S TX`

Loopback mode adds:
`I2S TX pins` -> `jumper wires` -> `I2S RX pins` -> `monitor metrics/logs`

## Board Integration

### Waveshare ESP32-C6 Touch Display Board

Current verified wiring for the external mono amplifier:

| Signal | GPIO | Note |
| --- | --- | --- |
| `WS / LRCLK` | `GPIO5` | Audio frame sync |
| `BCLK` | `GPIO6` | Audio bit clock |
| `DOUT` | `GPIO7` | Serial PCM output |
| `AMP SD` | not used in firmware | Shutdown tied high in hardware |

### Audio Format

The currently verified transmit format is:

| Parameter | Value |
| --- | --- |
| Sample rate | `44.1 kHz` |
| Data width | `16-bit PCM` |
| Slot width | `32-bit` |
| Framing | `MSB` |
| Channel mode | stereo output, mono content duplicated when needed |

## Error Handling

- SPIFFS mount errors stop startup and return `esp_err_t`.
- MP3 file open failure logs and returns `ESP_ERR_NOT_FOUND`.
- Decode buffer allocation failure returns `ESP_ERR_NO_MEM`.
- I2S write/read errors are logged and surfaced as `esp_err_t`.
- Playback stop uses TX disable instead of silence streaming, so silent stop depends on successful `sc_audio_i2s_set_tx_enabled(false)`.

## Constraints and Assumptions

- Playback rate is fixed at 44.1 kHz; non-44.1 kHz MP3 plays without resampling.
- Input MP3 path is fixed to `/spiffs/test.mp3`.
- SPIFFS `storage` partition is only 0x50000 bytes, so test assets must stay within that partition budget.
- Loopback is not wired on the current Waveshare ESP32-C6 board configuration.
- Current amplifier hardware is mono; stereo input is preserved on the bus but may be summed in the external amplifier stage.

## Verification Notes

- Verified on hardware:
  - clean generated sine tone on current I2S mapping
  - clean MP3 playback from `/spiffs/test.mp3`
  - playback start/stop behaves correctly
- Expected end-of-file log:
  - `sc_audio_mp3: finished playback: /spiffs/test.mp3`

## Open Items

- Add hidden display-based volume control.
- Prefer SD-backed playback when SD support is integrated, while keeping SPIFFS as a fallback.
- Add resampling or dynamic I2S reconfiguration for sample-rate mismatch.
- Add explicit pause semantics distinct from stop/mute.
