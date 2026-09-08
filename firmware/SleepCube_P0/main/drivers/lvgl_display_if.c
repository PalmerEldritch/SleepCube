#include "lvgl_display_if.h"

#include <math.h>
#include <stdlib.h>

#include "app_core.h"
#include "audio_service.h"
#include "board_pins.h"
#include "esp_check.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lcd_panel_if.h"
#include "light_service.h"

#if CONFIG_SC_BOARD_WAVESHARE_ESP32C6 && CONFIG_SC_UI_BACKEND_LCD_TOUCH

static const char *TAG = "sc_lvgl_if";

LV_FONT_DECLARE(lv_font_montserrat_28);

#define SC_LCD_DRAW_BUFFER_HEIGHT      50
#define SC_AMBIENT_TIMER_MS            24
#define SC_AMBIENT_PULSE_PERIOD_S      6.8f
#define SC_REST_LONG_PRESS_MS          1000U
#define SC_REST_MOVE_CANCEL_PX         12
#define SC_SLIDER_TIMEOUT_MS           3000U
#define SC_SLIDER_FADE_MS              180U
#define SC_SLIDER_PAD_X                10
#define SC_SLIDER_PAD_Y                28
#define SC_SLIDER_ZONE_BORDER_OPA      12
#define SC_AUDIO_ON_PULSE_BIAS         1.0f
#define SC_AUDIO_OFF_PULSE_BIAS       -1.0f
#define SC_AUDIO_PULSE_DAMPING_PER_S   0.85f
#define SC_LCD_BACKLIGHT_MAX_LEVEL     1023.0f
#define SC_LCD_BASE_BACKLIGHT_MIN      0.10f
#define SC_LCD_BASE_BACKLIGHT_MAX      0.72f
#define SC_LCD_BREATHING_DEPTH         0.065f
#define SC_LCD_AUDIO_ON_BOOST          0.035f
#define SC_LCD_PULSE_ON_MIN_DELTA      0.24f
#define SC_LCD_PULSE_ON_HEADROOM_GAIN  0.28f
#define SC_LCD_PULSE_OFF_MIN_DELTA     0.18f
#define SC_LCD_PULSE_OFF_LEVEL_GAIN    0.16f
#define SC_LCD_AMBIENT_FILTER_RATE     2.2f

typedef enum {
    SC_UI_MODE_REST = 0,
    SC_UI_MODE_SLIDER,
} sc_ui_mode_t;

typedef enum {
    SC_SLIDER_ROLE_VOLUME = 0,
    SC_SLIDER_ROLE_BRIGHTNESS,
} sc_slider_role_t;

static lv_display_t *s_display;
static lv_indev_t *s_touch_indev;
static bool s_started;
static lv_timer_t *s_ambient_timer;
static lv_timer_t *s_slider_timeout_timer;
static lv_obj_t *s_rest_surface;
static lv_obj_t *s_slider_overlay;
static lv_obj_t *s_slider_divider;
static lv_obj_t *s_volume_zone;
static lv_obj_t *s_brightness_zone;
static lv_obj_t *s_volume_slider;
static lv_obj_t *s_brightness_slider;
static sc_ui_mode_t s_ui_mode = SC_UI_MODE_REST;
static float s_ambient_phase;
static float s_audio_toggle_pulse_bias;
static uint8_t s_screen_brightness_percent = 30U;
static bool s_rest_press_tracking;
static bool s_slider_wait_release;
static bool s_slider_wait_repress;
static uint32_t s_rest_press_start_ms;
static lv_point_t s_rest_press_start_point;
static uint32_t s_ambient_last_tick_ms;
static float s_ambient_level_filtered;
static bool s_backlight_level_valid;

static float sc_clampf(float value, float lo, float hi)
{
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

static void sc_emit_event(sc_app_event_type_t type, int32_t value)
{
    sc_app_event_t evt = {
        .type = type,
        .value = value,
    };
    esp_err_t err = sc_app_core_post_event(&evt);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "event post failed: %s", esp_err_to_name(err));
    }
}

static void sc_slider_overlay_anim_cb(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void sc_slider_overlay_anim_ready_cb(lv_anim_t *anim)
{
    lv_obj_t *overlay = (lv_obj_t *)anim->var;
    if (anim->end_value == 0) {
        lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    }
}

static void sc_slider_overlay_fade(bool show)
{
    if (s_slider_overlay == NULL) {
        return;
    }

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, s_slider_overlay);
    lv_anim_set_exec_cb(&anim, sc_slider_overlay_anim_cb);
    lv_anim_set_values(&anim,
                       show ? 0 : lv_obj_get_style_opa(s_slider_overlay, 0),
                       show ? LV_OPA_COVER : 0);
    lv_anim_set_time(&anim, SC_SLIDER_FADE_MS);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&anim, sc_slider_overlay_anim_ready_cb);

    if (show) {
        lv_obj_clear_flag(s_slider_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(s_slider_overlay, LV_OPA_TRANSP, 0);
    }

    lv_anim_start(&anim);
}

static void sc_update_slider_visuals(void)
{
    if ((s_volume_slider == NULL) || (s_brightness_slider == NULL)) {
        return;
    }

    lv_slider_set_value(s_volume_slider, (int32_t)sc_audio_service_get_volume_percent(), LV_ANIM_OFF);
    lv_slider_set_value(s_brightness_slider, (int32_t)s_screen_brightness_percent, LV_ANIM_OFF);
}

static void sc_reset_slider_timeout(void)
{
    if (s_slider_timeout_timer == NULL) {
        return;
    }

    lv_timer_resume(s_slider_timeout_timer);
    lv_timer_reset(s_slider_timeout_timer);
}

static void sc_exit_slider_mode(void);

static void sc_slider_timeout_cb(lv_timer_t *timer)
{
    (void)timer;
    sc_exit_slider_mode();
}

static void sc_enter_slider_mode(void)
{
    if (s_ui_mode == SC_UI_MODE_SLIDER) {
        sc_reset_slider_timeout();
        return;
    }

    s_ui_mode = SC_UI_MODE_SLIDER;
    s_rest_press_tracking = false;
    s_slider_wait_release = true;
    s_slider_wait_repress = false;
    sc_update_slider_visuals();
    sc_slider_overlay_fade(true);
    sc_reset_slider_timeout();
    lv_indev_reset(NULL, s_rest_surface);
    ESP_LOGI(TAG, "ui mode -> slider");
}

static void sc_exit_slider_mode(void)
{
    if (s_ui_mode == SC_UI_MODE_REST) {
        return;
    }

    s_ui_mode = SC_UI_MODE_REST;
    s_slider_wait_release = false;
    s_slider_wait_repress = false;
    if (s_slider_timeout_timer != NULL) {
        lv_timer_pause(s_slider_timeout_timer);
    }
    sc_slider_overlay_fade(false);
    lv_indev_reset(NULL, s_rest_surface);
    ESP_LOGI(TAG, "ui mode -> rest");
}

static void sc_trigger_audio_toggle_pulse(bool audio_enabled)
{
    s_audio_toggle_pulse_bias = audio_enabled ? SC_AUDIO_ON_PULSE_BIAS : SC_AUDIO_OFF_PULSE_BIAS;
}

static uint8_t sc_zone_point_to_percent(const lv_obj_t *zone, const lv_point_t *point)
{
    lv_area_t coords;
    lv_obj_get_coords(zone, &coords);

    const int32_t height = (coords.y2 - coords.y1);
    if (height <= 0) {
        return 0;
    }

    const int32_t rel_y = point->y - coords.y1;
    const float normalized = 1.0f - ((float)rel_y / (float)height);
    return (uint8_t)(sc_clampf(normalized, 0.0f, 1.0f) * 100.0f);
}

static void sc_ambient_anim_cb(lv_timer_t *timer)
{
    (void)timer;

    if (!s_started) {
        return;
    }

    const uint32_t now_ms = lv_tick_get();
    float dt_s = (float)SC_AMBIENT_TIMER_MS / 1000.0f;
    if (s_ambient_last_tick_ms != 0U) {
        uint32_t elapsed_ms = lv_tick_elaps(s_ambient_last_tick_ms);
        if (elapsed_ms < 5U) {
            elapsed_ms = 5U;
        } else if (elapsed_ms > 80U) {
            elapsed_ms = 80U;
        }
        dt_s = (float)elapsed_ms / 1000.0f;
    }
    s_ambient_last_tick_ms = now_ms;

    s_ambient_phase += (2.0f * (float)M_PI * dt_s / SC_AMBIENT_PULSE_PERIOD_S);

    const float phase_a = s_ambient_phase;
    const float brightness_wave = 0.5f + (0.5f * sinf(phase_a));
    const float selected_level = (float)s_screen_brightness_percent / 100.0f;
    const float audio_boost = sc_audio_service_get_playback_enabled() ? SC_LCD_AUDIO_ON_BOOST : 0.0f;
    const float base_level = SC_LCD_BASE_BACKLIGHT_MIN +
                             ((SC_LCD_BASE_BACKLIGHT_MAX - SC_LCD_BASE_BACKLIGHT_MIN) * selected_level);
    const float ambient_target = sc_clampf(base_level +
                                               (SC_LCD_BREATHING_DEPTH * brightness_wave) +
                                               audio_boost,
                                           0.0f,
                                           1.0f);

    if (!s_backlight_level_valid) {
        s_ambient_level_filtered = ambient_target;
        s_backlight_level_valid = true;
    } else {
        const float filter_alpha = sc_clampf(dt_s * SC_LCD_AMBIENT_FILTER_RATE, 0.0f, 1.0f);
        s_ambient_level_filtered += (ambient_target - s_ambient_level_filtered) * filter_alpha;
    }

    float pulse_delta = 0.0f;
    if (s_audio_toggle_pulse_bias > 0.0f) {
        const float headroom = 1.0f - s_ambient_level_filtered;
        pulse_delta = s_audio_toggle_pulse_bias *
                      (SC_LCD_PULSE_ON_MIN_DELTA + (SC_LCD_PULSE_ON_HEADROOM_GAIN * headroom));
    } else if (s_audio_toggle_pulse_bias < 0.0f) {
        pulse_delta = s_audio_toggle_pulse_bias *
                      (SC_LCD_PULSE_OFF_MIN_DELTA + (SC_LCD_PULSE_OFF_LEVEL_GAIN * s_ambient_level_filtered));
    }

    const float backlight_output = sc_clampf(s_ambient_level_filtered + pulse_delta, 0.0f, 1.0f);
    const uint16_t backlight_level = (uint16_t)(backlight_output * SC_LCD_BACKLIGHT_MAX_LEVEL);
    (void)sc_lcd_panel_if_set_backlight_level(backlight_level);

    if (fabsf(s_audio_toggle_pulse_bias) > 0.002f) {
        s_audio_toggle_pulse_bias *= expf(-SC_AUDIO_PULSE_DAMPING_PER_S * dt_s);
    } else {
        s_audio_toggle_pulse_bias = 0.0f;
    }
}

static void sc_rest_surface_event_cb(lv_event_t *e)
{
    if (s_ui_mode != SC_UI_MODE_REST) {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev == NULL) {
            return;
        }

        s_rest_press_tracking = true;
        s_rest_press_start_ms = lv_tick_get();
        lv_indev_get_point(indev, &s_rest_press_start_point);
    } else if (code == LV_EVENT_PRESSING) {
        if (!s_rest_press_tracking) {
            return;
        }

        lv_indev_t *indev = lv_indev_get_act();
        if (indev == NULL) {
            return;
        }

        lv_point_t point;
        lv_indev_get_point(indev, &point);
        if ((LV_ABS(point.x - s_rest_press_start_point.x) > SC_REST_MOVE_CANCEL_PX) ||
            (LV_ABS(point.y - s_rest_press_start_point.y) > SC_REST_MOVE_CANCEL_PX)) {
            s_rest_press_tracking = false;
            return;
        }

        if (lv_tick_elaps(s_rest_press_start_ms) >= SC_REST_LONG_PRESS_MS) {
            sc_enter_slider_mode();
        }
    } else if (code == LV_EVENT_CLICKED) {
        const bool audio_enabled_next = !sc_audio_service_get_playback_enabled();
        sc_emit_event(SC_APP_EVT_UI_AUDIO_TOGGLE, 0);
        sc_trigger_audio_toggle_pulse(audio_enabled_next);
    } else if ((code == LV_EVENT_RELEASED) || (code == LV_EVENT_PRESS_LOST)) {
        s_rest_press_tracking = false;
    }
}

static void sc_slider_event_cb(lv_event_t *e)
{
    if (s_ui_mode != SC_UI_MODE_SLIDER) {
        return;
    }

    lv_obj_t *zone = lv_event_get_target(e);
    const sc_slider_role_t role = (sc_slider_role_t)(intptr_t)lv_event_get_user_data(e);
    const lv_event_code_t code = lv_event_get_code(e);

    if (s_slider_wait_release) {
        if ((code == LV_EVENT_RELEASED) || (code == LV_EVENT_PRESS_LOST)) {
            s_slider_wait_release = false;
            s_slider_wait_repress = true;
            sc_reset_slider_timeout();
        }
        return;
    }

    if (s_slider_wait_repress) {
        if (code == LV_EVENT_PRESSED) {
            s_slider_wait_repress = false;
            sc_reset_slider_timeout();
        }
        return;
    }

    if ((code == LV_EVENT_PRESSED) || (code == LV_EVENT_PRESSING) ||
        (code == LV_EVENT_RELEASED) || (code == LV_EVENT_CLICKED)) {
        sc_reset_slider_timeout();
    }

    if ((code != LV_EVENT_PRESSED) && (code != LV_EVENT_PRESSING) && (code != LV_EVENT_CLICKED)) {
        return;
    }

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    const uint8_t percent = sc_zone_point_to_percent(zone, &point);
    if (role == SC_SLIDER_ROLE_VOLUME) {
        (void)sc_audio_service_set_volume_percent(percent);
        lv_slider_set_value(s_volume_slider, percent, LV_ANIM_OFF);
    } else {
        s_screen_brightness_percent = percent;
        (void)sc_light_service_set_brightness_percent(percent);
        lv_slider_set_value(s_brightness_slider, percent, LV_ANIM_OFF);
    }
}

static lv_obj_t *sc_create_slider_zone(lv_obj_t *parent, lv_coord_t x, lv_coord_t width)
{
    lv_obj_t *zone = lv_obj_create(parent);
    lv_obj_remove_style_all(zone);
    lv_obj_set_pos(zone, x, 0);
    lv_obj_set_size(zone, width, SC_LCD_V_RES);
    lv_obj_add_flag(zone, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(zone, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(zone, 31, 0);
    lv_obj_set_style_bg_color(zone, lv_color_hex(0xF4E6D2), 0);
    return zone;
}

static lv_obj_t *sc_create_slider(lv_obj_t *parent, sc_slider_role_t role)
{
    lv_obj_t *slider = lv_slider_create(parent);
    const char *knob_symbol = (role == SC_SLIDER_ROLE_VOLUME) ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_EYE_OPEN;

    lv_obj_set_size(slider,
                    (SC_LCD_H_RES / 2) - (2 * SC_SLIDER_PAD_X),
                    SC_LCD_V_RES - (2 * SC_SLIDER_PAD_Y));
    lv_obj_center(slider);
    lv_slider_set_range(slider, 0, 100);

    lv_obj_set_style_radius(slider, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, 89, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xD7C1A3), LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(slider, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_border_color(slider, lv_color_hex(0xB8946E), LV_PART_MAIN);

    lv_obj_set_style_bg_opa(slider, LV_OPA_70, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xC99762), LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 18, LV_PART_INDICATOR);

    lv_obj_set_style_bg_opa(slider, 217, LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xFFF2E0), LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_outline_width(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(slider, 10, LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(slider, LV_OPA_20, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(slider, lv_color_hex(0x8C6540), LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 10, LV_PART_KNOB);
    lv_obj_set_style_bg_img_src(slider, knob_symbol, LV_PART_KNOB);
    lv_obj_set_style_bg_img_opa(slider, LV_OPA_80, LV_PART_KNOB);
    lv_obj_set_style_bg_img_recolor(slider, lv_color_hex(0x6D4526), LV_PART_KNOB);
    lv_obj_set_style_bg_img_recolor_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_text_color(slider, lv_color_hex(0x6D4526), LV_PART_KNOB);
    lv_obj_set_style_text_font(slider, &lv_font_montserrat_28, LV_PART_KNOB);
    return slider;
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

void sc_lvgl_display_create_ambient_screen(void)
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
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFE4B8), 0);
    lv_obj_set_style_bg_grad_color(lv_scr_act(), lv_color_hex(0xFFE4B8), 0);
    lv_obj_set_style_bg_grad_dir(lv_scr_act(), LV_GRAD_DIR_NONE, 0);
    s_screen_brightness_percent = sc_light_service_get_target_brightness_percent();
    s_slider_wait_release = false;
    s_slider_wait_repress = false;
    s_audio_toggle_pulse_bias = 0.0f;
    s_ambient_last_tick_ms = 0U;
    s_backlight_level_valid = false;

    s_rest_surface = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(s_rest_surface);
    lv_obj_set_pos(s_rest_surface, 0, 0);
    lv_obj_set_size(s_rest_surface, SC_LCD_H_RES, SC_LCD_V_RES);
    lv_obj_add_flag(s_rest_surface, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_rest_surface, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(s_rest_surface, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(s_rest_surface, sc_rest_surface_event_cb, LV_EVENT_ALL, NULL);

    s_slider_overlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(s_slider_overlay);
    lv_obj_set_pos(s_slider_overlay, 0, 0);
    lv_obj_set_size(s_slider_overlay, SC_LCD_H_RES, SC_LCD_V_RES);
    lv_obj_set_style_bg_opa(s_slider_overlay, 46, 0);
    lv_obj_set_style_bg_color(s_slider_overlay, lv_color_hex(0xF8EAD4), 0);
    lv_obj_set_style_opa(s_slider_overlay, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(s_slider_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_slider_overlay, LV_OBJ_FLAG_SCROLLABLE);

    s_volume_zone = sc_create_slider_zone(s_slider_overlay, 0, SC_LCD_H_RES / 2);
    s_brightness_zone = sc_create_slider_zone(s_slider_overlay, SC_LCD_H_RES / 2, SC_LCD_H_RES - (SC_LCD_H_RES / 2));

    s_slider_divider = lv_obj_create(s_slider_overlay);
    lv_obj_remove_style_all(s_slider_divider);
    lv_obj_set_size(s_slider_divider, 1, SC_LCD_V_RES - 40);
    lv_obj_set_pos(s_slider_divider, (SC_LCD_H_RES / 2), 20);
    lv_obj_set_style_bg_opa(s_slider_divider, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(s_slider_divider, lv_color_hex(0xC5A27D), 0);

    s_volume_slider = sc_create_slider(s_volume_zone, SC_SLIDER_ROLE_VOLUME);
    s_brightness_slider = sc_create_slider(s_brightness_zone, SC_SLIDER_ROLE_BRIGHTNESS);
    lv_obj_add_flag(s_volume_slider, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(s_brightness_slider, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_clear_flag(s_volume_slider, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_brightness_slider, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_volume_zone, sc_slider_event_cb, LV_EVENT_ALL, (void *)(intptr_t)SC_SLIDER_ROLE_VOLUME);
    lv_obj_add_event_cb(s_brightness_zone, sc_slider_event_cb, LV_EVENT_ALL, (void *)(intptr_t)SC_SLIDER_ROLE_BRIGHTNESS);

    sc_update_slider_visuals();

    if (s_ambient_timer == NULL) {
        s_ambient_timer = lv_timer_create(sc_ambient_anim_cb, SC_AMBIENT_TIMER_MS, NULL);
    }
    sc_ambient_anim_cb(NULL);

    if (s_slider_timeout_timer == NULL) {
        s_slider_timeout_timer = lv_timer_create(sc_slider_timeout_cb, SC_SLIDER_TIMEOUT_MS, NULL);
        lv_timer_pause(s_slider_timeout_timer);
    }

    lvgl_port_unlock();
    ESP_LOGI(TAG, "lvgl ambient screen created");
}

bool sc_lvgl_display_bus_lock(uint32_t timeout_ms)
{
    if (!s_started) {
        return true;
    }
    return lvgl_port_lock(timeout_ms);
}

void sc_lvgl_display_bus_unlock(void)
{
    if (!s_started) {
        return;
    }
    lvgl_port_unlock();
}

#else

esp_err_t sc_lvgl_display_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

void sc_lvgl_display_create_ambient_screen(void)
{
}

bool sc_lvgl_display_bus_lock(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return true;
}

void sc_lvgl_display_bus_unlock(void)
{
}

#endif
