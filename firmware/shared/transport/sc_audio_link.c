#include "sc_audio_link.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sc_frame.h"

#define SC_LINK_UART UART_NUM_1
#define SC_LINK_BAUD 115200
#define SC_LINK_READ_TIMEOUT_MS 20U
#define SC_LINK_WRITE_TIMEOUT_MS 100U
#define SC_LINK_RESPONSE_TIMEOUT_MS 250U
#define SC_LINK_MAX_ATTEMPTS 3U
#define SC_LINK_RECONNECT_MS 1000U
#define SC_LINK_STATUS_POLL_MS 1000U

typedef enum {
    CONTROL_REQUEST_STATUS = 0,
    CONTROL_REQUEST_PLAY,
    CONTROL_REQUEST_STOP,
    CONTROL_REQUEST_SET_VOLUME,
    CONTROL_REQUEST_REPLAY_LAST,
} control_request_type_t;

typedef struct {
    control_request_type_t type;
    uint32_t value;
} control_request_t;

typedef struct {
    bool started;
    volatile bool synchronized;
    uint32_t control_session_id;
    uint32_t audio_session_id;
    uint32_t next_command_id;
    uint16_t next_sequence;
    QueueHandle_t request_queue;
    SemaphoreHandle_t status_mutex;
    sc_audio_status_t status;
    sc_frame_t last_state_request;
    bool last_state_request_valid;
} control_context_t;

typedef struct {
    uint32_t audio_session_id;
    uint32_t minimum_duration_seconds;
    uint32_t maximum_duration_seconds;
    sc_audio_backend_t backend;
} audio_context_t;

static const char *TAG = "audio_link";
static bool s_started;
static control_context_t s_control;
static audio_context_t s_audio;

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000LL);
}

static uint32_t nonzero_random(void)
{
    const uint32_t value = esp_random();
    return value == 0U ? 1U : value;
}

static uint16_t next_sequence(void)
{
    uint16_t sequence = s_control.next_sequence++;
    if (sequence == 0U) {
        sequence = s_control.next_sequence++;
    }
    return sequence;
}

static uint32_t payload_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static esp_err_t configure_uart(int tx_gpio, int rx_gpio)
{
    if (s_started || tx_gpio < 0 || rx_gpio < 0 || tx_gpio == rx_gpio) {
        return ESP_ERR_INVALID_ARG;
    }
    const uart_config_t config = {
        .baud_rate = SC_LINK_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t error = uart_param_config(SC_LINK_UART, &config);
    if (error != ESP_OK) {
        return error;
    }
    error = uart_set_pin(SC_LINK_UART, tx_gpio, rx_gpio,
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (error != ESP_OK) {
        return error;
    }
    return uart_driver_install(SC_LINK_UART, 512, 0, 0, NULL, 0);
}

static esp_err_t send_frame(const sc_frame_t *frame)
{
    uint8_t wire[SC_FRAME_MAX_WIRE];
    size_t wire_len = 0;
    if (!sc_frame_encode(frame, wire, &wire_len)) {
        return ESP_ERR_INVALID_ARG;
    }
    const int written = uart_write_bytes(SC_LINK_UART, wire, wire_len);
    if (written != (int)wire_len) {
        return ESP_FAIL;
    }
    return uart_wait_tx_done(SC_LINK_UART,
                             pdMS_TO_TICKS(SC_LINK_WRITE_TIMEOUT_MS));
}

static bool material_status_changed(const sc_audio_status_t *left,
                                    const sc_audio_status_t *right)
{
    return left->audio_session_id != right->audio_session_id ||
           left->state != right->state ||
           left->volume_percent != right->volume_percent ||
           left->stop_reason != right->stop_reason ||
           left->flags != right->flags ||
           left->fault_code != right->fault_code;
}

static bool backend_status(sc_audio_status_t *status)
{
    memset(status, 0, sizeof(*status));
    status->audio_session_id = s_audio.audio_session_id;
    status->state = SC_AUDIO_STATE_NOT_READY;
    status->stop_reason = SC_AUDIO_STOP_REASON_BOOT;
    if (s_audio.backend.get_status == NULL) {
        return true;
    }
    const sc_audio_result_t result = s_audio.backend.get_status(
        s_audio.backend.context, status);
    status->audio_session_id = s_audio.audio_session_id;
    if (result != SC_AUDIO_RESULT_ACCEPTED) {
        status->state = SC_AUDIO_STATE_FAULT;
        status->stop_reason = SC_AUDIO_STOP_REASON_FAULT;
        status->fault_code = 1U;
    }
    return true;
}

static void audio_task(void *argument)
{
    (void)argument;
    sc_audio_server_t server;
    sc_audio_server_init(&server, s_audio.audio_session_id,
                         s_audio.minimum_duration_seconds,
                         s_audio.maximum_duration_seconds,
                         &s_audio.backend);
    sc_frame_parser_t parser;
    sc_frame_parser_init(&parser);
    sc_audio_status_t previous_status = {0};
    bool previous_valid = false;
    uint32_t rejected = 0U;

    ESP_LOGI(TAG,
             "Audio endpoint ready session=%08" PRIx32
             " uart=%d baud=%d duration=%" PRIu32 "..%" PRIu32 "s",
             s_audio.audio_session_id, SC_LINK_UART, SC_LINK_BAUD,
             s_audio.minimum_duration_seconds,
             s_audio.maximum_duration_seconds);
    for (;;) {
        uint8_t bytes[64];
        const int count = uart_read_bytes(SC_LINK_UART, bytes, sizeof(bytes),
                                          pdMS_TO_TICKS(SC_LINK_READ_TIMEOUT_MS));
        const uint32_t timestamp_ms = now_ms();
        if (count < 0) {
            ESP_LOGE(TAG, "Audio UART receive failed");
            vTaskDelete(NULL);
            return;
        }
        if (sc_frame_parser_expire(&parser, timestamp_ms) == SC_FRAME_TIMEOUT) {
            ESP_LOGW(TAG, "Audio discarded truncated frame count=%" PRIu32,
                     ++rejected);
        }
        for (int i = 0; i < count; ++i) {
            sc_frame_t request = {0};
            const sc_frame_result_t parse_result = sc_frame_parser_feed(
                &parser, bytes[i], timestamp_ms, &request);
            if (parse_result == SC_FRAME_READY) {
                sc_frame_t response;
                if (sc_audio_server_handle(&server, &request, &response)) {
                    const esp_err_t error = send_frame(&response);
                    if (error != ESP_OK) {
                        ESP_LOGE(TAG, "%s response tx failed: %s",
                                 sc_audio_message_name(response.type),
                                 esp_err_to_name(error));
                    } else {
                        ESP_LOGI(TAG, "rx=%s seq=%u tx=%s",
                                 sc_audio_message_name(request.type),
                                 request.sequence,
                                 sc_audio_message_name(response.type));
                    }
                }
            } else if (parse_result != SC_FRAME_NONE) {
                ESP_LOGW(TAG, "Audio rejected frame reason=%d count=%" PRIu32,
                         (int)parse_result, ++rejected);
            }
        }

        sc_audio_status_t current_status;
        backend_status(&current_status);
        if (!previous_valid ||
            material_status_changed(&previous_status, &current_status)) {
            const uint8_t event_type = sc_audio_event_type(
                previous_valid ? &previous_status : NULL,
                &current_status, !previous_valid);
            sc_frame_t event;
            if (sc_audio_make_status_event(&event, event_type,
                                           &current_status)) {
                const esp_err_t error = send_frame(&event);
                if (error == ESP_OK) {
                    ESP_LOGI(TAG, "event=%s state=%s volume=%u%% remaining=%" PRIu32 "s",
                             sc_audio_message_name(event_type),
                             sc_audio_state_name(current_status.state),
                             current_status.volume_percent,
                             current_status.remaining_seconds);
                }
            }
            previous_status = current_status;
            previous_valid = true;
        }
    }
}

static void set_control_status(const sc_audio_status_t *status)
{
    if (xSemaphoreTake(s_control.status_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        s_control.status = *status;
        xSemaphoreGive(s_control.status_mutex);
    }
    ESP_LOGI(TAG, "remote state=%s volume=%u%% remaining=%" PRIu32
                  "s reason=%u fault=%u",
             sc_audio_state_name(status->state), status->volume_percent,
             status->remaining_seconds, (unsigned)status->stop_reason,
             status->fault_code);
}

static bool build_control_request(const control_request_t *request,
                                  sc_frame_t *frame)
{
    const uint16_t sequence = next_sequence();
    switch (request->type) {
    case CONTROL_REQUEST_STATUS:
        return sc_audio_make_get_status(frame, sequence);
    case CONTROL_REQUEST_PLAY:
        if (!sc_audio_make_play(frame, sequence,
                                s_control.control_session_id,
                                s_control.audio_session_id,
                                s_control.next_command_id++, request->value)) {
            return false;
        }
        break;
    case CONTROL_REQUEST_STOP:
        if (!sc_audio_make_stop(frame, sequence,
                                s_control.control_session_id,
                                s_control.audio_session_id,
                                s_control.next_command_id++)) {
            return false;
        }
        break;
    case CONTROL_REQUEST_SET_VOLUME:
        if (!sc_audio_make_set_volume(frame, sequence,
                                      s_control.control_session_id,
                                      s_control.audio_session_id,
                                      s_control.next_command_id++,
                                      (uint8_t)request->value)) {
            return false;
        }
        break;
    case CONTROL_REQUEST_REPLAY_LAST:
        if (!s_control.last_state_request_valid) {
            return false;
        }
        *frame = s_control.last_state_request;
        frame->sequence = sequence;
        return true;
    default:
        return false;
    }
    s_control.last_state_request = *frame;
    s_control.last_state_request_valid = true;
    return true;
}

static void process_control_frame(const sc_frame_t *frame,
                                  const sc_frame_t *pending,
                                  bool *pending_valid,
                                  bool *need_hello,
                                  bool *need_status)
{
    if (frame->sequence == 0U) {
        sc_audio_status_t event_status;
        if (sc_audio_parse_status(frame, &event_status)) {
            if (s_control.audio_session_id != 0U &&
                event_status.audio_session_id != s_control.audio_session_id) {
                ESP_LOGW(TAG,
                         "Audio reset/session change detected old=%08" PRIx32
                         " new=%08" PRIx32 "; abandoning pending command",
                         s_control.audio_session_id,
                         event_status.audio_session_id);
                s_control.synchronized = false;
                *pending_valid = false;
                *need_hello = true;
                *need_status = false;
            } else {
                set_control_status(&event_status);
                ESP_LOGI(TAG, "event=%s", sc_audio_message_name(frame->type));
            }
        }
        return;
    }
    if (!*pending_valid || frame->sequence != pending->sequence) {
        ESP_LOGW(TAG, "unmatched response type=%s seq=%u",
                 sc_audio_message_name(frame->type), frame->sequence);
        return;
    }

    if (frame->type == SC_AUDIO_MSG_ERROR) {
        ESP_LOGE(TAG, "correlated ERROR response to %s seq=%u code=%u",
                 sc_audio_message_name(pending->type), frame->sequence,
                 frame->payload_len >= 2U ? frame->payload[1] : 0xffU);
        if (pending->type == SC_AUDIO_MSG_HELLO ||
            pending->type == SC_AUDIO_MSG_GET_STATUS) {
            s_control.synchronized = false;
            *need_hello = true;
        } else {
            *need_status = true;
        }
        *pending_valid = false;
        return;
    }

    if (pending->type == SC_AUDIO_MSG_HELLO) {
        sc_audio_info_t info;
        if (sc_audio_parse_info(frame, &info) &&
            info.control_session_id == s_control.control_session_id &&
            info.semantic_version == SC_AUDIO_SEMANTIC_VERSION &&
            (info.capabilities & (SC_AUDIO_CAP_FINITE_PLAY |
                                  SC_AUDIO_CAP_VOLUME |
                                  SC_AUDIO_CAP_STATUS)) ==
                (SC_AUDIO_CAP_FINITE_PLAY |
                 SC_AUDIO_CAP_VOLUME |
                 SC_AUDIO_CAP_STATUS)) {
            s_control.audio_session_id = info.audio_session_id;
            ESP_LOGI(TAG,
                     "INFO synchronized audio_session=%08" PRIx32
                     " capabilities=0x%04x duration=%" PRIu32 "..%" PRIu32 "s",
                     info.audio_session_id, info.capabilities,
                     info.minimum_duration_seconds,
                     info.maximum_duration_seconds);
            *need_status = true;
        } else {
            ESP_LOGE(TAG, "invalid/incompatible INFO response");
            *need_hello = true;
        }
    } else if (pending->type == SC_AUDIO_MSG_GET_STATUS) {
        sc_audio_status_t status;
        if (frame->type == SC_AUDIO_MSG_STATUS &&
            sc_audio_parse_status(frame, &status) &&
            status.audio_session_id == s_control.audio_session_id) {
            set_control_status(&status);
            s_control.synchronized = true;
            ESP_LOGI(TAG, "startup/status reconciliation complete");
        } else {
            ESP_LOGE(TAG, "invalid STATUS response");
            *need_hello = true;
        }
    } else {
        uint8_t request_type;
        sc_audio_result_t result;
        uint32_t command_id;
        sc_audio_status_t status;
        if (sc_audio_parse_result(frame, &request_type, &result,
                                  &command_id, &status) &&
            request_type == pending->type &&
            pending->payload_len >= 12U &&
            command_id == payload_u32(&pending->payload[8])) {
            ESP_LOGI(TAG, "RESULT request=%s command=%" PRIu32 " result=%s",
                     sc_audio_message_name(request_type), command_id,
                     sc_audio_result_name(result));
            if (status.audio_session_id == s_control.audio_session_id) {
                set_control_status(&status);
            }
            if (result == SC_AUDIO_RESULT_SESSION_MISMATCH) {
                s_control.synchronized = false;
                *need_hello = true;
                *need_status = false;
            }
        } else {
            ESP_LOGE(TAG, "invalid RESULT response");
            *need_status = true;
        }
    }
    *pending_valid = false;
}

static bool send_new_pending(sc_frame_t *pending,
                             bool *pending_valid,
                             uint8_t *attempts,
                             uint32_t *deadline_ms)
{
    const esp_err_t error = send_frame(pending);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "%s tx failed: %s",
                 sc_audio_message_name(pending->type), esp_err_to_name(error));
        return false;
    }
    *pending_valid = true;
    *attempts = 1U;
    *deadline_ms = now_ms() + SC_LINK_RESPONSE_TIMEOUT_MS;
    ESP_LOGI(TAG, "tx=%s seq=%u attempt=1",
             sc_audio_message_name(pending->type), pending->sequence);
    return true;
}

static void control_task(void *argument)
{
    (void)argument;
    sc_frame_parser_t parser;
    sc_frame_parser_init(&parser);
    sc_frame_t pending = {0};
    bool pending_valid = false;
    bool need_hello = true;
    bool need_status = false;
    uint8_t attempts = 0U;
    uint32_t deadline_ms = 0U;
    uint32_t reconnect_due_ms = 0U;
    uint32_t status_poll_due_ms = 0U;
    uint32_t rejected = 0U;

    ESP_LOGI(TAG,
             "Control client ready session=%08" PRIx32
             " uart=%d baud=%d retry=%umsx%u",
             s_control.control_session_id, SC_LINK_UART, SC_LINK_BAUD,
             SC_LINK_RESPONSE_TIMEOUT_MS, SC_LINK_MAX_ATTEMPTS);
    for (;;) {
        uint8_t bytes[64];
        const int count = uart_read_bytes(SC_LINK_UART, bytes, sizeof(bytes),
                                          pdMS_TO_TICKS(SC_LINK_READ_TIMEOUT_MS));
        const uint32_t timestamp_ms = now_ms();
        if (count < 0) {
            ESP_LOGE(TAG, "Control UART receive failed");
            vTaskDelete(NULL);
            return;
        }
        if (sc_frame_parser_expire(&parser, timestamp_ms) == SC_FRAME_TIMEOUT) {
            ESP_LOGW(TAG, "Control discarded truncated frame count=%" PRIu32,
                     ++rejected);
        }
        for (int i = 0; i < count; ++i) {
            sc_frame_t frame = {0};
            const sc_frame_result_t result = sc_frame_parser_feed(
                &parser, bytes[i], timestamp_ms, &frame);
            if (result == SC_FRAME_READY) {
                process_control_frame(&frame, &pending, &pending_valid,
                                      &need_hello,
                                      &need_status);
            } else if (result != SC_FRAME_NONE) {
                ESP_LOGW(TAG, "Control rejected frame reason=%d count=%" PRIu32,
                         (int)result, ++rejected);
            }
        }

        if (pending_valid && (int32_t)(timestamp_ms - deadline_ms) >= 0) {
            if (attempts < SC_LINK_MAX_ATTEMPTS) {
                ++attempts;
                const esp_err_t error = send_frame(&pending);
                deadline_ms = timestamp_ms + SC_LINK_RESPONSE_TIMEOUT_MS;
                ESP_LOGW(TAG, "retry=%s seq=%u attempt=%u result=%s",
                         sc_audio_message_name(pending.type), pending.sequence,
                         attempts, esp_err_to_name(error));
            } else {
                ESP_LOGE(TAG, "%s seq=%u timed out after %u attempts; state unknown",
                         sc_audio_message_name(pending.type), pending.sequence,
                         attempts);
                pending_valid = false;
                s_control.synchronized = false;
                need_hello = true;
                need_status = false;
                reconnect_due_ms = timestamp_ms + SC_LINK_RECONNECT_MS;
            }
        }

        if (!pending_valid && need_hello &&
            (int32_t)(timestamp_ms - reconnect_due_ms) >= 0) {
            sc_audio_make_hello(&pending, next_sequence(),
                                s_control.control_session_id);
            if (send_new_pending(&pending, &pending_valid, &attempts,
                                 &deadline_ms)) {
                need_hello = false;
            }
        } else if (!pending_valid && need_status) {
            sc_audio_make_get_status(&pending, next_sequence());
            if (send_new_pending(&pending, &pending_valid, &attempts,
                                 &deadline_ms)) {
                need_status = false;
                status_poll_due_ms = timestamp_ms + SC_LINK_STATUS_POLL_MS;
            }
        } else if (!pending_valid && s_control.synchronized) {
            control_request_t request;
            if (xQueueReceive(s_control.request_queue, &request, 0) == pdTRUE) {
                if (build_control_request(&request, &pending)) {
                    send_new_pending(&pending, &pending_valid, &attempts,
                                     &deadline_ms);
                } else {
                    ESP_LOGW(TAG, "diagnostic request unavailable type=%d",
                             (int)request.type);
                }
            } else if ((int32_t)(timestamp_ms - status_poll_due_ms) >= 0) {
                sc_audio_make_get_status(&pending, next_sequence());
                if (send_new_pending(&pending, &pending_valid, &attempts,
                                     &deadline_ms)) {
                    status_poll_due_ms = timestamp_ms +
                                         SC_LINK_STATUS_POLL_MS;
                }
            }
        }
    }
}

esp_err_t sc_audio_link_start_audio(int tx_gpio,
                                    int rx_gpio,
                                    uint32_t audio_session_id,
                                    uint32_t minimum_duration_seconds,
                                    uint32_t maximum_duration_seconds,
                                    const sc_audio_backend_t *backend)
{
    if (backend == NULL || audio_session_id == 0U ||
        minimum_duration_seconds == 0U ||
        maximum_duration_seconds < minimum_duration_seconds) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t error = configure_uart(tx_gpio, rx_gpio);
    if (error != ESP_OK) {
        return error;
    }
    s_audio.audio_session_id = audio_session_id;
    s_audio.minimum_duration_seconds = minimum_duration_seconds;
    s_audio.maximum_duration_seconds = maximum_duration_seconds;
    s_audio.backend = *backend;
    if (xTaskCreate(audio_task, "audio_link_rx", 6144, NULL, 5, NULL) != pdPASS) {
        uart_driver_delete(SC_LINK_UART);
        return ESP_ERR_NO_MEM;
    }
    s_started = true;
    ESP_LOGI(TAG, "UART configured tx=GPIO%d rx=GPIO%d role=audio",
             tx_gpio, rx_gpio);
    return ESP_OK;
}

esp_err_t sc_audio_link_start_control(int tx_gpio, int rx_gpio)
{
    esp_err_t error = configure_uart(tx_gpio, rx_gpio);
    if (error != ESP_OK) {
        return error;
    }
    memset(&s_control, 0, sizeof(s_control));
    s_control.control_session_id = nonzero_random();
    s_control.next_command_id = 1U;
    s_control.next_sequence = 1U;
    s_control.status.state = SC_AUDIO_STATE_NOT_READY;
    s_control.request_queue = xQueueCreate(8, sizeof(control_request_t));
    s_control.status_mutex = xSemaphoreCreateMutex();
    if (s_control.request_queue == NULL || s_control.status_mutex == NULL) {
        if (s_control.request_queue != NULL) {
            vQueueDelete(s_control.request_queue);
        }
        if (s_control.status_mutex != NULL) {
            vSemaphoreDelete(s_control.status_mutex);
        }
        uart_driver_delete(SC_LINK_UART);
        return ESP_ERR_NO_MEM;
    }
    if (xTaskCreate(control_task, "audio_link_rx", 6144, NULL, 5, NULL) != pdPASS) {
        vQueueDelete(s_control.request_queue);
        vSemaphoreDelete(s_control.status_mutex);
        uart_driver_delete(SC_LINK_UART);
        return ESP_ERR_NO_MEM;
    }
    s_control.started = true;
    s_started = true;
    ESP_LOGI(TAG, "UART configured tx=GPIO%d rx=GPIO%d role=control",
             tx_gpio, rx_gpio);
    return ESP_OK;
}

static esp_err_t queue_control_request(control_request_type_t type,
                                       uint32_t value)
{
    if (!s_control.started || s_control.request_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_control.synchronized) {
        return ESP_ERR_INVALID_STATE;
    }
    const control_request_t request = {.type = type, .value = value};
    return xQueueSend(s_control.request_queue, &request, 0) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t sc_audio_link_control_play(uint32_t duration_seconds)
{
    return queue_control_request(CONTROL_REQUEST_PLAY, duration_seconds);
}

esp_err_t sc_audio_link_control_stop(void)
{
    return queue_control_request(CONTROL_REQUEST_STOP, 0U);
}

esp_err_t sc_audio_link_control_set_volume(uint8_t volume_percent)
{
    if (volume_percent > 100U) {
        return ESP_ERR_INVALID_ARG;
    }
    return queue_control_request(CONTROL_REQUEST_SET_VOLUME, volume_percent);
}

esp_err_t sc_audio_link_control_request_status(void)
{
    return queue_control_request(CONTROL_REQUEST_STATUS, 0U);
}

esp_err_t sc_audio_link_control_replay_last(void)
{
    if (!s_control.last_state_request_valid) {
        return ESP_ERR_INVALID_STATE;
    }
    return queue_control_request(CONTROL_REQUEST_REPLAY_LAST, 0U);
}

bool sc_audio_link_control_is_synchronized(void)
{
    return s_control.synchronized;
}

esp_err_t sc_audio_link_control_get_status(sc_audio_status_t *status)
{
    if (!s_control.started || status == NULL || s_control.status_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_control.status_mutex, pdMS_TO_TICKS(20)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    *status = s_control.status;
    xSemaphoreGive(s_control.status_mutex);
    return ESP_OK;
}
