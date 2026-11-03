#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "audio_output.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "wav_reader.h"
#include "sc_audio_protocol.h"

typedef enum {
    SC_PLAYBACK_STOPPED = 0,
    SC_PLAYBACK_STARTING,
    SC_PLAYBACK_PLAYING,
    SC_PLAYBACK_STOPPING,
    SC_PLAYBACK_FAULT,
} sc_playback_state_t;

typedef enum {
    SC_PLAYBACK_COMMAND_TOGGLE = 0,
    SC_PLAYBACK_COMMAND_CYCLE_VOLUME,
    SC_PLAYBACK_COMMAND_PLAY,
    SC_PLAYBACK_COMMAND_STOP,
    SC_PLAYBACK_COMMAND_SET_VOLUME,
    SC_PLAYBACK_COMMAND_SET_SOURCE,
    SC_PLAYBACK_COMMAND_SET_TONE_NOTE,
    SC_PLAYBACK_COMMAND_SET_I2S_PACKING,
} sc_playback_command_type_t;

typedef struct {
    sc_playback_command_type_t type;
    uint32_t duration_seconds;
    uint8_t volume_percent;
    sc_audio_source_t source;
    sc_audio_tone_note_t tone_note;
    sc_audio_i2s_packing_t i2s_packing;
} sc_playback_command_t;

typedef enum {
    SC_PLAYBACK_STOP_REASON_BOOT = 0,
    SC_PLAYBACK_STOP_REASON_MANUAL,
    SC_PLAYBACK_STOP_REASON_TIMER_EXPIRED,
    SC_PLAYBACK_STOP_REASON_FAULT,
} sc_playback_stop_reason_t;

typedef struct {
    sc_playback_state_t state;
    uint8_t volume_percent;
    sc_audio_source_t source;
    sc_audio_tone_note_t tone_note;
    sc_audio_i2s_packing_t i2s_packing;
    sc_playback_stop_reason_t stop_reason;
    uint32_t remaining_seconds;
    uint16_t fault_code;
} sc_playback_snapshot_t;

typedef struct {
    sc_audio_output_t *output;
    sc_wav_reader_t *reader;
    QueueHandle_t command_queue;
    SemaphoreHandle_t status_mutex;
    TaskHandle_t task;
    volatile sc_playback_state_t state;
    uint8_t volume_percent;
    sc_audio_source_t source;
    sc_audio_tone_note_t tone_note;
    sc_audio_i2s_packing_t i2s_packing;
    sc_playback_stop_reason_t stop_reason;
    int64_t session_deadline_us;
    uint16_t fault_code;
} sc_playback_controller_t;

/** Start the sole owner task for decoder, gain, session, and output state. */
esp_err_t sc_playback_controller_init(sc_playback_controller_t *controller,
                                      sc_audio_output_t *output,
                                      sc_wav_reader_t *reader,
                                      uint8_t initial_volume_percent);

/** Queue a non-blocking standalone-test command. */
esp_err_t sc_playback_controller_send(sc_playback_controller_t *controller,
                                      sc_playback_command_type_t command);

/** Queue an explicit finite PLAY request from the semantic protocol. */
esp_err_t sc_playback_controller_play(sc_playback_controller_t *controller,
                                      uint32_t duration_seconds);

/** Queue an idempotent STOP request from the semantic protocol. */
esp_err_t sc_playback_controller_stop(sc_playback_controller_t *controller);

/** Queue an authoritative persistent volume request in the 0..100 domain. */
esp_err_t sc_playback_controller_set_volume(sc_playback_controller_t *controller,
                                            uint8_t volume_percent);

/** Queue development-only volatile profile, tone and slot-packing controls. */
esp_err_t sc_playback_controller_set_source(sc_playback_controller_t *controller,
                                            sc_audio_source_t source);
esp_err_t sc_playback_controller_set_tone_note(sc_playback_controller_t *controller,
                                               sc_audio_tone_note_t note);
esp_err_t sc_playback_controller_set_i2s_packing(
    sc_playback_controller_t *controller,
    sc_audio_i2s_packing_t packing);

/** Read a synchronized authoritative state snapshot without touching audio I/O. */
esp_err_t sc_playback_controller_get_snapshot(sc_playback_controller_t *controller,
                                              sc_playback_snapshot_t *snapshot);

const char *sc_playback_state_name(sc_playback_state_t state);
