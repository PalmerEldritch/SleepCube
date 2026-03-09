# SleepCube P0 Audio Playback Implementation

## Purpose

Provide a firmware PoC that reads MP3 data from onboard flash (SPIFFS), decodes to PCM,
and transmits PCM over I2S. Optional digital loopback verifies signal integrity without
the amplifier board connected.

## Scope

- Included:
  - Audio control service (`start`, `toggle`, `set playback`, `volume step`)
  - SPIFFS mount of partition label `storage`
  - MP3 file playback from `/spiffs/test.mp3`
  - Helix MP3 decode to 16-bit PCM
  - I2S TX output at fixed 44.1 kHz, 16-bit stereo
  - Software mute path by continuously writing silence while playback is disabled
  - Optional I2S RX loopback monitor (`CONFIG_SC_LOOPBACK_ENABLE`)
- Excluded:
  - Resampling for non-44.1 kHz MP3 content
  - Track selection / playlist management
  - Analog output validation through MAX98357

## Runtime Behavior

1. `sc_audio_service_start()` initializes audio subsystem and sets defaults (`enabled=0`, `volume=70%`).
2. `sc_audio_player_start()` mounts SPIFFS, starts optional loopback task, and initializes I2S TX.
3. Audio task runs continuously:
   - When disabled: writes silence blocks to I2S (output stays quiet and clocking remains stable).
   - When enabled: decodes `/spiffs/test.mp3` and writes PCM frames to I2S.
4. End-of-file or stop request exits decode loop; task resumes idle/silence mode.

## Interfaces

| API | Input | Output | Notes |
| --- | --- | --- | --- |
| `sc_audio_service_start()` | none | `esp_err_t` | Starts service and sets defaults |
| `sc_audio_service_set_playback(enable)` | bool | `esp_err_t` | Explicit playback control |
| `sc_audio_service_toggle_playback()` | none | `esp_err_t` | Toggle playback state |
| `sc_audio_service_change_volume(delta_steps)` | signed step | `esp_err_t` | Volume step is 5% per step |
| `sc_audio_player_start()` | none | `esp_err_t` | Subsystem startup orchestration |
| `sc_audio_fs_mount()` | none | `esp_err_t` | Mounts SPIFFS partition `storage` |
| `sc_audio_mp3_play_file(path)` | MP3 path | `esp_err_t` | Decode and playback loop body |
| `sc_audio_i2s_init(rate)` | sample rate | `esp_err_t` | TX init |
| `sc_audio_i2s_write(buf, frames, timeout)` | interleaved stereo PCM | `esp_err_t` | Frame write to TX |
| `sc_audio_i2s_init_rx(rate)` | sample rate | `esp_err_t` | RX init for loopback |
| `sc_audio_i2s_read(buf, frames, out, timeout)` | RX frame request | `esp_err_t` | Frame read from RX |
| `sc_audio_loopback_start(rate)` | sample rate | `esp_err_t` | Starts verification task |

## Configuration

| Key | Default | Effect |
| --- | --- | --- |
| `CONFIG_SC_ENABLE_AUDIO` | `y` | Builds and starts audio service |
| `CONFIG_SC_LOOPBACK_ENABLE` | `y` | Enables loopback task and RX channel init |
| Partition table `storage` | 0x50000 | Space for SPIFFS audio files |
| I2S sample rate | 44100 Hz | TX and loopback RX operating rate |

## Data Flow

`SPIFFS file` -> `fread()` chunks -> `Helix MP3 decoder` -> `PCM frames` -> `I2S TX`

Loopback mode adds:
`I2S TX pins` -> `jumper wires` -> `I2S RX pins` -> `monitor metrics/logs`

## Error Handling

- SPIFFS mount errors stop startup and return `esp_err_t`.
- MP3 file open failure logs and returns `ESP_ERR_NOT_FOUND`.
- Decode buffer allocation failure returns `ESP_ERR_NO_MEM`.
- I2S write/read errors are logged and surfaced as `esp_err_t`.

## Constraints and Assumptions

- Playback rate is fixed at 44.1 kHz; non-44.1 kHz MP3 plays without resampling.
- Input MP3 path is fixed to `/spiffs/test.mp3`.
- Loopback requires external jumper wiring per `board_pins.h`.
- Board target is ESP32 DevKitC pin mapping currently hard-coded for I2S.

## Verification Notes

- Expected loopback logs:
  - `signal` near 100%
  - `stereo_mismatch` near 0%
  - `est_freq` follows test tone/frequency content
- End-of-file log:
  - `sc_audio_mp3: finished playback: /spiffs/test.mp3`

## Open Items

- Make playback path configurable in `menuconfig`.
- Add volume ramping and click/pop suppression.
- Add resampling or dynamic I2S reconfiguration for sample-rate mismatch.
- Add explicit pause semantics distinct from stop/mute.
