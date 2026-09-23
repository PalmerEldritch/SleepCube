#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "sc_audio_protocol.h"

/** Start the Audio endpoint. The supplied backend must remain valid. */
esp_err_t sc_audio_link_start_audio(int tx_gpio,
                                    int rx_gpio,
                                    uint32_t audio_session_id,
                                    uint32_t minimum_duration_seconds,
                                    uint32_t maximum_duration_seconds,
                                    const sc_audio_backend_t *backend);

/** Start the Control client, which immediately performs HELLO/GET_STATUS. */
esp_err_t sc_audio_link_start_control(int tx_gpio, int rx_gpio);

/** Non-blocking Control-side semantic requests. */
esp_err_t sc_audio_link_control_play(uint32_t duration_seconds);
esp_err_t sc_audio_link_control_stop(void);
esp_err_t sc_audio_link_control_set_volume(uint8_t volume_percent);
esp_err_t sc_audio_link_control_request_status(void);

/** M4 diagnostic hook: resend the last state-changing request without reissuing it. */
esp_err_t sc_audio_link_control_replay_last(void);

bool sc_audio_link_control_is_synchronized(void);
esp_err_t sc_audio_link_control_get_status(sc_audio_status_t *status);
