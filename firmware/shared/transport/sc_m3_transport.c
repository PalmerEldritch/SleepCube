#include "sc_m3_transport.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sc_frame.h"

#define SC_M3_UART UART_NUM_1
#define SC_M3_BAUD 115200
#define SC_M3_PING_INTERVAL_MS 2000U
#define SC_M3_READ_TIMEOUT_MS 20U
#define SC_M3_WRITE_TIMEOUT_MS 100U

static const char *TAG = "m3_transport";
static bool s_started;
static sc_m3_role_t s_role;

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000LL);
}

static esp_err_t write_wire(const uint8_t *wire, size_t wire_len)
{
    const int written = uart_write_bytes(SC_M3_UART, wire, wire_len);
    if (written != (int)wire_len) {
        return ESP_FAIL;
    }
    return uart_wait_tx_done(SC_M3_UART,
                             pdMS_TO_TICKS(SC_M3_WRITE_TIMEOUT_MS));
}

static esp_err_t send_frame(const sc_frame_t *frame)
{
    uint8_t wire[SC_FRAME_MAX_WIRE];
    size_t wire_len = 0;
    if (!sc_frame_encode(frame, wire, &wire_len)) {
        return ESP_ERR_INVALID_ARG;
    }
    return write_wire(wire, wire_len);
}

static void transport_task(void *arg)
{
    (void)arg;
    const char *role_name = s_role == SC_M3_ROLE_CONTROL ? "control" : "audio";
    sc_frame_parser_t parser;
    sc_frame_parser_init(&parser);
    uint16_t next_sequence = s_role == SC_M3_ROLE_CONTROL ? 1U : 0x8001U;
    uint16_t pending_sequence = 0;
    uint32_t pending_token = 0;
    bool pending = false;
    uint32_t next_ping_ms = now_ms() + SC_M3_PING_INTERVAL_MS +
                            (s_role == SC_M3_ROLE_AUDIO ? 500U : 0U);
    uint32_t rejected = 0;
    uint32_t matched = 0;
    uint8_t truncated_wire[SC_FRAME_MAX_WIRE];
    size_t truncated_len = 0;
    uint32_t truncated_tail_due_ms = 0;
    bool probes_sent = false;
    bool truncated_tail_pending = false;

    ESP_LOGI(TAG, "role=%s ready uart=%d baud=%d framing=COBS/CRC16",
             role_name, SC_M3_UART, SC_M3_BAUD);
    for (;;) {
        uint8_t bytes[64];
        const int count = uart_read_bytes(SC_M3_UART, bytes, sizeof(bytes),
                                         pdMS_TO_TICKS(SC_M3_READ_TIMEOUT_MS));
        if (count < 0) {
            ESP_LOGE(TAG, "role=%s UART RX failed; diagnostic task stopped",
                     role_name);
            break;
        }
        const uint32_t timestamp_ms = now_ms();
        if (sc_frame_parser_expire(&parser, timestamp_ms) == SC_FRAME_TIMEOUT) {
            ++rejected;
            ESP_LOGW(TAG, "role=%s truncated frame discarded count=%" PRIu32,
                     role_name, rejected);
        }
        for (int i = 0; i < count; ++i) {
            sc_frame_t frame = {0};
            const sc_frame_result_t result = sc_frame_parser_feed(
                &parser, bytes[i], timestamp_ms, &frame);
            if (result == SC_FRAME_READY) {
                if (frame.type == SC_FRAME_TYPE_M3_PING &&
                    frame.payload_len == 4U) {
                    frame.type = SC_FRAME_TYPE_M3_PONG;
                    const esp_err_t error = send_frame(&frame);
                    if (error != ESP_OK) {
                        ESP_LOGE(TAG, "role=%s PONG tx failed: %s", role_name,
                                 esp_err_to_name(error));
                    } else {
                        ESP_LOGI(TAG, "role=%s echoed PING seq=%u", role_name,
                                 frame.sequence);
                    }
                } else if (frame.type == SC_FRAME_TYPE_M3_PONG &&
                           frame.payload_len == 4U && pending &&
                           frame.sequence == pending_sequence &&
                           frame.payload[0] == (uint8_t)pending_token &&
                           frame.payload[1] == (uint8_t)(pending_token >> 8) &&
                           frame.payload[2] == (uint8_t)(pending_token >> 16) &&
                           frame.payload[3] == (uint8_t)(pending_token >> 24)) {
                    pending = false;
                    ++matched;
                    ESP_LOGI(TAG, "role=%s PONG matched seq=%u total=%" PRIu32,
                             role_name, frame.sequence, matched);
                } else {
                    ++rejected;
                    ESP_LOGW(TAG,
                             "role=%s unsupported/unmatched type=0x%02x seq=%u rejected=%" PRIu32,
                             role_name, frame.type, frame.sequence, rejected);
                }
            } else if (result != SC_FRAME_NONE) {
                ++rejected;
                ESP_LOGW(TAG, "role=%s rejected frame reason=%d count=%" PRIu32,
                         role_name, (int)result, rejected);
            }
        }

        /* Once the link is live, inject one CRC fault and one timed-out partial
         * packet per direction. The peer must reject both and then match a
         * subsequent valid PING; no product command types exist in M3. */
        if (matched >= 3U && !probes_sent) {
            sc_frame_t probe = {
                .type = SC_FRAME_TYPE_M3_PING,
                .sequence = next_sequence++,
                .payload_len = 4U,
                .payload = {0x4dU, 0x33U, 0x54U, 0x53U},
            };
            uint8_t corrupt_wire[SC_FRAME_MAX_WIRE];
            size_t corrupt_len = 0;
            if (sc_frame_encode(&probe, corrupt_wire, &corrupt_len) &&
                sc_frame_encode(&probe, truncated_wire, &truncated_len)) {
                /* Byte 2 is the nonzero type byte for this version-1 probe;
                 * changing it preserves COBS but invalidates the CRC. */
                corrupt_wire[2] ^= 0x01U;
                const esp_err_t crc_result = write_wire(corrupt_wire, corrupt_len);
                const esp_err_t partial_result = write_wire(
                    truncated_wire, truncated_len / 2U);
                if (crc_result == ESP_OK && partial_result == ESP_OK) {
                    truncated_tail_pending = true;
                    truncated_tail_due_ms = timestamp_ms +
                                            SC_FRAME_INTERBYTE_TIMEOUT_MS + 50U;
                    ESP_LOGI(TAG, "role=%s malformed CRC and partial probes sent",
                             role_name);
                } else {
                    ESP_LOGE(TAG, "role=%s malformed probe TX failed: %s/%s",
                             role_name, esp_err_to_name(crc_result),
                             esp_err_to_name(partial_result));
                }
            }
            probes_sent = true;
        }
        if (truncated_tail_pending &&
            (int32_t)(timestamp_ms - truncated_tail_due_ms) >= 0) {
            const size_t head_len = truncated_len / 2U;
            const esp_err_t error = write_wire(&truncated_wire[head_len],
                                                truncated_len - head_len);
            ESP_LOGI(TAG, "role=%s truncated probe tail sent result=%s",
                     role_name, esp_err_to_name(error));
            truncated_tail_pending = false;
        }

        if ((int32_t)(timestamp_ms - next_ping_ms) >= 0) {
            if (pending) {
                ESP_LOGW(TAG, "role=%s PONG missing seq=%u", role_name,
                         pending_sequence);
            }
            sc_frame_t ping = {
                .type = SC_FRAME_TYPE_M3_PING,
                .sequence = next_sequence++,
                .payload_len = 4U,
            };
            const uint32_t token = timestamp_ms;
            ping.payload[0] = (uint8_t)token;
            ping.payload[1] = (uint8_t)(token >> 8);
            ping.payload[2] = (uint8_t)(token >> 16);
            ping.payload[3] = (uint8_t)(token >> 24);
            const esp_err_t error = send_frame(&ping);
            if (error == ESP_OK) {
                pending = true;
                pending_sequence = ping.sequence;
                pending_token = token;
                ESP_LOGI(TAG, "role=%s PING sent seq=%u", role_name,
                         ping.sequence);
            } else {
                pending = false;
                ESP_LOGE(TAG, "role=%s PING tx failed: %s", role_name,
                         esp_err_to_name(error));
            }
            next_ping_ms = timestamp_ms + SC_M3_PING_INTERVAL_MS;
        }
    }
    vTaskDelete(NULL);
}

esp_err_t sc_m3_transport_start(sc_m3_role_t role, int tx_gpio, int rx_gpio)
{
    if (s_started || (role != SC_M3_ROLE_CONTROL && role != SC_M3_ROLE_AUDIO) ||
        tx_gpio < 0 || rx_gpio < 0 || tx_gpio == rx_gpio) {
        return ESP_ERR_INVALID_ARG;
    }
    const uart_config_t uart_config = {
        .baud_rate = SC_M3_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t error = uart_param_config(SC_M3_UART, &uart_config);
    if (error != ESP_OK) {
        return error;
    }
    error = uart_set_pin(SC_M3_UART, tx_gpio, rx_gpio,
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (error != ESP_OK) {
        return error;
    }
    error = uart_driver_install(SC_M3_UART, 512, 0, 0, NULL, 0);
    if (error != ESP_OK) {
        return error;
    }
    s_role = role;
    if (xTaskCreate(transport_task, "m3_uart", 4096, NULL, 4, NULL) != pdPASS) {
        uart_driver_delete(SC_M3_UART);
        return ESP_ERR_NO_MEM;
    }
    s_started = true;
    ESP_LOGI(TAG, "UART configured tx=GPIO%d rx=GPIO%d role=%s",
             tx_gpio, rx_gpio,
             role == SC_M3_ROLE_CONTROL ? "control" : "audio");
    return ESP_OK;
}
