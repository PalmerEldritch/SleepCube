#include "lvgl_display_if.h"

#include <math.h>

#include "audio_service.h"
#include "board_pins.h"
#include "esp_check.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lcd_panel_if.h"

static const char *TAG = "sc_lvgl_if";

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6 && CONFIG_SC_UI_BACKEND_LCD_TOUCH

#define SC_LCD_DRAW_BUFFER_HEIGHT  50
#define SC_AMBIENT_TIMER_MS        20
#define SC_AMBIENT_PULSE_PERIOD_S  2.0f

static lv_display_t *s_display;
static lv_indev_t *s_touch_indev;
static bool s_started;
static lv_obj_t *s_tap_pulse;
static lv_timer_t *s_touch_poll_timer;
static lv_timer_t *s_ambient_timer;
static bool s_touch_pressed;
static uint16_t s_touch_x;
static uint16_t s_touch_y;
static float s_ambient_phase;
static float s_dither_err_r;
static float s_dither_err_g;
static float s_dither_err_b;

static void sc_lvgl_touch_poll_cb(lv_timer_t *timer)
{
    (void)timer;

    esp_lcd_touch_handle_t touch = sc_lcd_panel_if_get_touch_handle();
    if (touch == NULL) {
        return;
    }

    uint16_t x[1];
    uint16_t y[1];
    uint8_t count = 0;

    esp_lcd_touch_read_data(touch);
    bool pressed = esp_lcd_touch_get_coordinates(touch, x, y, NULL, &count, 1);

    if (pressed && count > 0) {
        s_touch_pressed = true;
        s_touch_x = x[0];
        s_touch_y = y[0];
    } else {
        s_touch_pressed = false;
    }
}

static lv_color_t sc_mix3(lv_color_t a, lv_color_t b, lv_color_t c, uint8_t mix_ab, uint8_t mix_bc)
{
    lv_color_t ab = lv_color_mix(a, b, mix_ab);
    return lv_color_mix(ab, c, mix_bc);
}

static lv_color_t sc_white_mix(uint8_t warm_mix, uint8_t cool_mix)
{
    return sc_mix3(lv_color_hex(0xFFF8F0),
                   lv_color_hex(0xF4EBDD),
                   lv_color_hex(0xEDF6FF),
                   warm_mix,
                   cool_mix);
}

static uint8_t sc_quantize_dither(float value, float *err_accum)
{
    float adjusted = value + *err_accum;
    if (adjusted < 0.0f) {
        adjusted = 0.0f;
    } else if (adjusted > 255.0f) {
        adjusted = 255.0f;
    }

    const uint8_t out = (uint8_t)(adjusted + 0.5f);
    *err_accum = adjusted - (float)out;
    return out;
}

static void sc_ambient_anim_cb(lv_timer_t *timer)
{
    (void)timer;

    if (s_tap_pulse == NULL) {
        return;
    }

    s_ambient_phase += (2.0f * (float)M_PI * ((float)SC_AMBIENT_TIMER_MS / 1000.0f) / SC_AMBIENT_PULSE_PERIOD_S);

    const float phase_a = s_ambient_phase;
    const float brightness_wave = 0.5f + 0.5f * sinf(phase_a);
    const float warmth_wave = 1.0f - brightness_wave;
    const float brightness = 0.62f + 0.34f * brightness_wave;
    const float warmth = 0.16f + 0.34f * warmth_wave;
    const float cool = 0.06f + 0.22f * brightness_wave;

    const float r_f = 255.0f * brightness;
    const float g_f = 255.0f * (brightness - (0.035f * warmth) + (0.010f * cool));
    const float b_f = 255.0f * (brightness - (0.115f * warmth) + (0.080f * cool));

    const uint8_t r = sc_quantize_dither(r_f, &s_dither_err_r);
    const uint8_t g = sc_quantize_dither(g_f, &s_dither_err_g);
    const uint8_t b = sc_quantize_dither(b_f, &s_dither_err_b);
    const lv_color_t base = lv_color_make(r, g, b);

    lv_obj_set_style_bg_color(lv_scr_act(), base, 0);
    lv_obj_set_style_bg_grad_color(lv_scr_act(), base, 0);
    lv_obj_set_style_bg_grad_dir(lv_scr_act(), LV_GRAD_DIR_NONE, 0);

    if (!lv_obj_has_flag(s_tap_pulse, LV_OBJ_FLAG_HIDDEN)) {
        lv_opa_t opa = lv_obj_get_style_border_opa(s_tap_pulse, 0);
        if (opa > 20) {
            lv_coord_t size = lv_obj_get_width(s_tap_pulse);
            lv_obj_set_size(s_tap_pulse, size + 2, size + 2);
            lv_obj_set_style_border_opa(s_tap_pulse, (lv_opa_t)(opa - 12), 0);
        } else {
            lv_obj_add_flag(s_tap_pulse, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void sc_root_tap_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(sc_audio_service_toggle_playback());

    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t point = {
        .x = SC_LCD_H_RES / 2,
        .y = SC_LCD_V_RES / 2,
    };
    if (indev != NULL) {
        lv_indev_get_point(indev, &point);
    } else if (s_touch_pressed) {
        point.x = s_touch_x;
        point.y = s_touch_y;
    }

    lv_obj_set_size(s_tap_pulse, 26, 26);
    lv_obj_set_pos(s_tap_pulse, point.x - 13, point.y - 13);
    lv_obj_set_style_border_opa(s_tap_pulse, LV_OPA_80, 0);
    lv_obj_clear_flag(s_tap_pulse, LV_OBJ_FLAG_HIDDEN);
}

esp_err_t sc_lvgl_display_init(void)
{
    if (s_started) {
        return ESP_OK;
    }

    esp_lcd_panel_io_handle_t io_handle = sc_lcd_panel_if_get_io_handle();
    esp_lcd_panel_handle_t panel_handle = sc_lcd_panel_if_get_panel_handle();
    esp_lcd_touch_handle_t touch_handle = sc_lcd_panel_if_get_touch_handle();

    ESP_RETURN_ON_FALSE(io_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "panel io handle not ready");
    ESP_RETURN_ON_FALSE(panel_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "panel handle not ready");
    ESP_RETURN_ON_FALSE(touch_handle != NULL, ESP_ERR_INVALID_STATE, TAG, "touch handle not ready");

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 1024 * 10,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5,
    };
    lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = SC_LCD_H_RES * SC_LCD_DRAW_BUFFER_HEIGHT,
        .double_buffer = true,
        .hres = SC_LCD_H_RES,
        .vres = SC_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        },
    };
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = NULL,
        .handle = touch_handle,
    };

    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl port init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(panel_handle, 34, 0), TAG, "panel gap set failed");

    s_display = lvgl_port_add_disp(&disp_cfg);
    ESP_RETURN_ON_FALSE(s_display != NULL, ESP_FAIL, TAG, "lvgl display add failed");

    lvgl_port_touch_cfg_t touch_cfg_local = touch_cfg;
    touch_cfg_local.disp = s_display;
    s_touch_indev = lvgl_port_add_touch(&touch_cfg_local);
    ESP_RETURN_ON_FALSE(s_touch_indev != NULL, ESP_FAIL, TAG, "lvgl touch add failed");

    s_started = true;
    ESP_LOGI(TAG, "lvgl display init complete");
    return ESP_OK;
}

void sc_lvgl_display_create_test_screen(void)
{
    if (!s_started) {
        return;
    }

    if (!lvgl_port_lock(0)) {
        ESP_LOGW(TAG, "lvgl lock failed");
        return;
    }

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x241A1B), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(lv_scr_act(), 0, 0);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(lv_scr_act(), sc_root_tap_cb, LV_EVENT_CLICKED, NULL);

    s_tap_pulse = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(s_tap_pulse);
    lv_obj_set_size(s_tap_pulse, 26, 26);
    lv_obj_set_style_radius(s_tap_pulse, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_tap_pulse, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_tap_pulse, 2, 0);
    lv_obj_set_style_border_color(s_tap_pulse, lv_color_hex(0xFFF4D8), 0);
    lv_obj_set_style_border_opa(s_tap_pulse, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(s_tap_pulse, LV_OBJ_FLAG_HIDDEN);

    if (s_touch_poll_timer == NULL) {
        s_touch_poll_timer = lv_timer_create(sc_lvgl_touch_poll_cb, 30, NULL);
    }
    sc_lvgl_touch_poll_cb(NULL);
    if (s_ambient_timer == NULL) {
        s_ambient_timer = lv_timer_create(sc_ambient_anim_cb, SC_AMBIENT_TIMER_MS, NULL);
    }
    sc_ambient_anim_cb(NULL);

    lvgl_port_unlock();
    ESP_LOGI(TAG, "lvgl test screen created");
}

#else

esp_err_t sc_lvgl_display_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

void sc_lvgl_display_create_test_screen(void)
{
}

#endif
