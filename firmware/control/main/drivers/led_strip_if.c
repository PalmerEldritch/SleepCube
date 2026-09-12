#include "led_strip_if.h"

#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "led_strip_encoder.h"

static const char *TAG = "sc_led_strip_if";

static rmt_channel_handle_t s_led_chan;
static rmt_encoder_handle_t s_led_encoder;
static rmt_transmit_config_t s_tx_config = {
    .loop_count = 0,
    .flags = {
        .queue_nonblocking = 1,
    },
};
static size_t s_led_count;
static sc_led_pixel_order_t s_pixel_order;
static uint8_t *s_tx_buf;
static volatile bool s_tx_busy;

#define SC_LED_RMT_RESOLUTION_HZ (10000000)

static bool sc_led_strip_tx_idle(void)
{
    return !s_tx_busy;
}

static bool sc_led_strip_on_tx_done(rmt_channel_handle_t channel, const rmt_tx_done_event_data_t *edata, void *user_ctx)
{
    (void)channel;
    (void)edata;
    (void)user_ctx;
    s_tx_busy = false;
    return false;
}

esp_err_t sc_led_strip_if_init(int gpio_num, size_t led_count, sc_led_pixel_order_t pixel_order)
{
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio_num,
        .mem_block_symbols = 64,
        .resolution_hz = SC_LED_RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    esp_err_t err = rmt_new_tx_channel(&tx_chan_config, &s_led_chan);
    if (err != ESP_OK) {
        return err;
    }

    sc_led_strip_encoder_config_t encoder_config = {
        .resolution = SC_LED_RMT_RESOLUTION_HZ,
    };
    err = sc_rmt_new_led_strip_encoder(&encoder_config, &s_led_encoder);
    if (err != ESP_OK) {
        return err;
    }

    err = rmt_enable(s_led_chan);
    if (err != ESP_OK) {
        return err;
    }
    rmt_tx_event_callbacks_t tx_cbs = {
        .on_trans_done = sc_led_strip_on_tx_done,
    };
    err = rmt_tx_register_event_callbacks(s_led_chan, &tx_cbs, NULL);
    if (err != ESP_OK) {
        return err;
    }

    s_led_count = led_count;
    s_pixel_order = pixel_order;
    s_tx_busy = false;
    s_tx_buf = (uint8_t *)malloc(led_count * 3U);
    if (s_tx_buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    memset(s_tx_buf, 0, led_count * 3U);

    ESP_LOGI(TAG, "led strip init: gpio=%d count=%u", gpio_num, (unsigned)led_count);
    return ESP_OK;
}

esp_err_t sc_led_strip_if_write_rgb(const uint8_t *rgb_data, size_t led_count)
{
    if ((s_led_chan == NULL) || (s_led_encoder == NULL) || (s_tx_buf == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!sc_led_strip_tx_idle()) {
        return ESP_ERR_TIMEOUT;
    }

    size_t n = led_count;
    if (n > s_led_count) {
        n = s_led_count;
    }

    for (size_t i = 0; i < n; i++) {
        const uint8_t r = rgb_data[(i * 3) + 0];
        const uint8_t g = rgb_data[(i * 3) + 1];
        const uint8_t b = rgb_data[(i * 3) + 2];
        if (s_pixel_order == SC_LED_PIXEL_ORDER_RGB) {
            s_tx_buf[(i * 3) + 0] = r;
            s_tx_buf[(i * 3) + 1] = g;
            s_tx_buf[(i * 3) + 2] = b;
        } else {
            s_tx_buf[(i * 3) + 0] = g;
            s_tx_buf[(i * 3) + 1] = r;
            s_tx_buf[(i * 3) + 2] = b;
        }
    }

    s_tx_busy = true;
    esp_err_t err = rmt_transmit(s_led_chan, s_led_encoder, s_tx_buf, n * 3U, &s_tx_config);
    if (err != ESP_OK) {
        s_tx_busy = false;
        return err;
    }
    return ESP_OK;
}

esp_err_t sc_led_strip_if_clear(void)
{
    if ((s_led_chan == NULL) || (s_led_encoder == NULL) || (s_tx_buf == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!sc_led_strip_tx_idle()) {
        return ESP_ERR_TIMEOUT;
    }

    memset(s_tx_buf, 0, s_led_count * 3U);
    s_tx_busy = true;
    esp_err_t err = rmt_transmit(s_led_chan, s_led_encoder, s_tx_buf, s_led_count * 3U, &s_tx_config);
    if (err != ESP_OK) {
        s_tx_busy = false;
        return err;
    }
    return ESP_OK;
}
