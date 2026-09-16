#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "audio_output.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "wav_reader.h"

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
} sc_playback_command_type_t;

typedef struct {
    sc_playback_command_type_t type;
} sc_playback_command_t;

typedef struct {
    sc_audio_output_t *output;
    sc_wav_reader_t *reader;
    QueueHandle_t command_queue;
    TaskHandle_t task;
    volatile sc_playback_state_t state;
    uint8_t volume_percent;
} sc_playback_controller_t;

/** Start the sole owner task for decoder, gain, session, and output state. */
esp_err_t sc_playback_controller_init(sc_playback_controller_t *controller,
                                      sc_audio_output_t *output,
                                      sc_wav_reader_t *reader,
                                      uint8_t initial_volume_percent);

/** Queue a non-blocking standalone-test command. */
esp_err_t sc_playback_controller_send(sc_playback_controller_t *controller,
                                      sc_playback_command_type_t command);

const char *sc_playback_state_name(sc_playback_state_t state);
