#include "playback_controller.h"

#include <inttypes.h>
#include <string.h>

#include "app_config.h"
#include "audio_settings.h"
#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
#include "audio_test_profiles.h"
#include "tone_generator.h"
#endif

typedef struct {
    int32_t start_q15;
    int32_t target_q15;
    uint32_t total_frames;
    uint32_t position;
} linear_ramp_t;

typedef struct {
    sc_playback_controller_t *controller;
    linear_ramp_t playback_ramp;
    linear_ramp_t volume_ramp;
    int32_t playback_gain_q15;
    int32_t volume_gain_q15;
    int64_t session_deadline_us;
    int64_t next_heartbeat_us;
    uint64_t session_frames;
    uint32_t loop_count;
    bool timeout_fade_started;
    bool source_switch_pending;
    bool source_switch_fading_in;
    sc_audio_source_t active_source;
    sc_audio_tone_note_t active_tone_note;
    sc_audio_i2s_packing_t active_i2s_packing;
    sc_audio_source_t requested_source;
    sc_audio_tone_note_t requested_tone_note;
    sc_audio_i2s_packing_t requested_i2s_packing;
    uint32_t active_sample_rate_hz;
    uint8_t active_data_bits;
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    sc_tone_generator_t tone_generator;
#endif
    sc_playback_stop_reason_t stop_reason;
} playback_runtime_t;

static const char *TAG = "playback";

static void update_shared_status(sc_playback_controller_t *controller,
                                 sc_playback_state_t state,
                                 sc_playback_stop_reason_t stop_reason,
                                 int64_t session_deadline_us,
                                 uint16_t fault_code)
{
    if (xSemaphoreTake(controller->status_mutex, portMAX_DELAY) == pdTRUE) {
        controller->state = state;
        controller->stop_reason = stop_reason;
        controller->session_deadline_us = session_deadline_us;
        controller->fault_code = fault_code;
        xSemaphoreGive(controller->status_mutex);
    }
}

static void update_shared_volume(sc_playback_controller_t *controller,
                                 uint8_t volume_percent)
{
    if (xSemaphoreTake(controller->status_mutex, portMAX_DELAY) == pdTRUE) {
        controller->volume_percent = volume_percent;
        xSemaphoreGive(controller->status_mutex);
    }
}

#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
static void update_shared_debug_config(sc_playback_controller_t *controller,
                                       sc_audio_source_t source,
                                       sc_audio_tone_note_t note,
                                       sc_audio_i2s_packing_t packing)
{
    if (xSemaphoreTake(controller->status_mutex, portMAX_DELAY) == pdTRUE) {
        controller->source = source;
        controller->tone_note = note;
        controller->i2s_packing = packing;
        xSemaphoreGive(controller->status_mutex);
    }
}
#endif

static uint32_t frames_for_ms(const playback_runtime_t *runtime,
                              uint32_t milliseconds)
{
    return (uint32_t)(((uint64_t)runtime->active_sample_rate_hz * milliseconds) /
                      1000U);
}

static int32_t volume_to_gain_q15(uint8_t volume_percent)
{
    const uint32_t squared = (uint32_t)volume_percent * volume_percent;
    return (int32_t)(((uint64_t)SC_VOLUME_OUTPUT_CAP_Q15 * squared) /
                     (SC_VOLUME_MAX_PERCENT * SC_VOLUME_MAX_PERCENT));
}

static void ramp_begin(linear_ramp_t *ramp,
                       int32_t start_q15,
                       int32_t target_q15,
                       uint32_t duration_frames)
{
    ramp->start_q15 = start_q15;
    ramp->target_q15 = target_q15;
    ramp->total_frames = duration_frames;
    ramp->position = 0;
}

static int32_t ramp_next(linear_ramp_t *ramp)
{
    if (ramp->position >= ramp->total_frames || ramp->total_frames == 0) {
        return ramp->target_q15;
    }
    ramp->position++;
    const int64_t delta = (int64_t)ramp->target_q15 - ramp->start_q15;
    return ramp->start_q15 +
           (int32_t)((delta * ramp->position) / ramp->total_frames);
}

static bool ramp_complete(const linear_ramp_t *ramp)
{
    return ramp->position >= ramp->total_frames;
}

const char *sc_playback_state_name(sc_playback_state_t state)
{
    switch (state) {
    case SC_PLAYBACK_STOPPED:
        return "STOPPED";
    case SC_PLAYBACK_STARTING:
        return "STARTING";
    case SC_PLAYBACK_PLAYING:
        return "PLAYING";
    case SC_PLAYBACK_STOPPING:
        return "STOPPING";
    case SC_PLAYBACK_FAULT:
        return "FAULT";
    default:
        return "UNKNOWN";
    }
}

static void enter_fault(playback_runtime_t *runtime,
                        const char *operation,
                        esp_err_t error)
{
    update_shared_status(runtime->controller, SC_PLAYBACK_FAULT,
                         SC_PLAYBACK_STOP_REASON_FAULT, 0,
                         (uint16_t)error);
    ESP_LOGE(TAG, "%s failed: %s; entering silent FAULT state",
             operation, esp_err_to_name(error));
    sc_audio_output_fail_safe(runtime->controller->output);
}

static esp_err_t fill_mono_block(playback_runtime_t *runtime,
                                 int32_t *mono,
                                 size_t requested_frames)
{
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    if (runtime->active_source == SC_AUDIO_SOURCE_TONE) {
        int16_t generated[SC_PLAYBACK_BLOCK_FRAMES];
        sc_tone_generator_fill(&runtime->tone_generator, generated,
                               requested_frames);
        for (size_t i = 0; i < requested_frames; ++i) {
            mono[i] = (int32_t)generated[i] * 65536;
        }
        return ESP_OK;
    }
#endif
    size_t filled_frames = 0;
    while (filled_frames < requested_frames) {
        size_t frames_read = 0;
        esp_err_t result = sc_wav_reader_read_q31(
            runtime->controller->reader,
            &mono[filled_frames],
            requested_frames - filled_frames,
            &frames_read);
        if (result != ESP_OK) {
            return result;
        }
        filled_frames += frames_read;
        if (filled_frames == requested_frames) {
            break;
        }

        result = sc_wav_reader_rewind(runtime->controller->reader);
        if (result != ESP_OK) {
            return result;
        }
        runtime->loop_count++;
        ESP_LOGI(TAG, "content loop=%" PRIu32, runtime->loop_count);
    }
    return ESP_OK;
}

#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
static uint8_t slot_bits_for(uint8_t data_bits,
                             sc_audio_i2s_packing_t packing)
{
    return packing == SC_AUDIO_I2S_PACKING_PADDED_32 ? 32U : data_bits;
}

static esp_err_t apply_debug_profile(playback_runtime_t *runtime,
                                     sc_audio_source_t source,
                                     sc_audio_tone_note_t note,
                                     sc_audio_i2s_packing_t packing)
{
    sc_audio_test_profile_t profile;
    if (!sc_audio_test_profile_get(source, &profile) ||
        note >= SC_AUDIO_TONE_COUNT ||
        packing >= SC_AUDIO_I2S_PACKING_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    sc_wav_reader_t candidate = {0};
    if (profile.path != NULL) {
        esp_err_t result = sc_wav_reader_open_expected(&candidate,
                                                       profile.path,
                                                       profile.sample_rate_hz,
                                                       profile.data_bits);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "profile file rejected: source=%u path=%s error=%s",
                     (unsigned)source, profile.path, esp_err_to_name(result));
            return result;
        }
    }

    const uint8_t requested_slot_bits =
        slot_bits_for(profile.data_bits, packing);
    const uint32_t previous_rate = runtime->active_sample_rate_hz;
    const uint8_t previous_data_bits = runtime->active_data_bits;
    const uint8_t previous_slot_bits =
        slot_bits_for(previous_data_bits, runtime->active_i2s_packing);
    esp_err_t result = sc_audio_output_reconfigure(runtime->controller->output,
                                                   profile.sample_rate_hz,
                                                   profile.data_bits,
                                                   requested_slot_bits);
    if (result != ESP_OK) {
        sc_wav_reader_close(&candidate);
        const esp_err_t restore_result = sc_audio_output_reconfigure(
            runtime->controller->output,
            previous_rate,
            previous_data_bits,
            previous_slot_bits);
        if (restore_result != ESP_OK) {
            ESP_LOGE(TAG, "previous I2S format restore failed: %s",
                     esp_err_to_name(restore_result));
        }
        return result;
    }

    if (profile.path != NULL) {
        sc_wav_reader_close(runtime->controller->reader);
        *runtime->controller->reader = candidate;
    }
    if (source == SC_AUDIO_SOURCE_TONE) {
        sc_tone_generator_set_note(&runtime->tone_generator, note);
    }

    runtime->active_source = source;
    runtime->active_tone_note = note;
    runtime->active_i2s_packing = packing;
    runtime->active_sample_rate_hz = profile.sample_rate_hz;
    runtime->active_data_bits = profile.data_bits;
    update_shared_debug_config(runtime->controller, source, note, packing);
    ESP_LOGI(TAG,
             "debug profile active: source=%u rate=%" PRIu32
             "Hz data=%ubit slot=%ubit",
             (unsigned)source, profile.sample_rate_hz, profile.data_bits,
             requested_slot_bits);
    return ESP_OK;
}
#endif

static void begin_playback(playback_runtime_t *runtime,
                           uint32_t duration_seconds)
{
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    if (runtime->active_source == SC_AUDIO_SOURCE_TONE) {
        sc_tone_generator_set_note(&runtime->tone_generator,
                                   runtime->active_tone_note);
    } else
#endif
    {
        const esp_err_t result =
            sc_wav_reader_rewind(runtime->controller->reader);
        if (result != ESP_OK) {
            enter_fault(runtime, "content rewind", result);
            return;
        }
    }

    runtime->session_frames = 0;
    runtime->loop_count = 0;
    runtime->timeout_fade_started = false;
    runtime->source_switch_pending = false;
    runtime->source_switch_fading_in = false;
    runtime->requested_source = runtime->active_source;
    runtime->requested_tone_note = runtime->active_tone_note;
    runtime->requested_i2s_packing = runtime->active_i2s_packing;
    runtime->playback_gain_q15 = 0;
    runtime->volume_gain_q15 =
        volume_to_gain_q15(runtime->controller->volume_percent);
    ramp_begin(&runtime->playback_ramp, 0, INT16_MAX,
               frames_for_ms(runtime, SC_PLAYBACK_START_FADE_MS));
    ramp_begin(&runtime->volume_ramp,
               runtime->volume_gain_q15,
               runtime->volume_gain_q15,
               0);

    const int64_t now_us = esp_timer_get_time();
    runtime->session_deadline_us = now_us +
        ((int64_t)duration_seconds * 1000000LL);
    runtime->next_heartbeat_us = now_us +
        ((int64_t)SC_PLAYBACK_HEARTBEAT_SECONDS * 1000000LL);
    runtime->stop_reason = SC_PLAYBACK_STOP_REASON_BOOT;
    update_shared_status(runtime->controller, SC_PLAYBACK_STARTING,
                         runtime->stop_reason, runtime->session_deadline_us, 0U);
    ESP_LOGI(TAG,
             "session start duration=%us volume=%u%% fade_in=%ums deadline_us=%" PRId64,
             duration_seconds,
             runtime->controller->volume_percent,
             SC_PLAYBACK_START_FADE_MS,
             runtime->session_deadline_us);
}

static void begin_stop(playback_runtime_t *runtime,
                       uint32_t fade_ms,
                       const char *reason,
                       sc_playback_stop_reason_t stop_reason)
{
    if (runtime->controller->state == SC_PLAYBACK_STOPPING) {
        return;
    }
    ramp_begin(&runtime->playback_ramp,
               runtime->playback_gain_q15,
               0,
               frames_for_ms(runtime, fade_ms));
    runtime->source_switch_pending = false;
    runtime->source_switch_fading_in = false;
    runtime->requested_source = runtime->active_source;
    runtime->requested_tone_note = runtime->active_tone_note;
    runtime->requested_i2s_packing = runtime->active_i2s_packing;
    runtime->stop_reason = stop_reason;
    update_shared_status(runtime->controller, SC_PLAYBACK_STOPPING,
                         stop_reason, runtime->session_deadline_us, 0U);
    ESP_LOGI(TAG, "session stop requested reason=%s fade_out=%ums",
             reason, fade_ms);
}

static void set_volume(playback_runtime_t *runtime, uint8_t next_volume)
{
    if (next_volume == runtime->controller->volume_percent) {
        ESP_LOGI(TAG, "volume already %u%%", next_volume);
        return;
    }
    const esp_err_t result = sc_audio_settings_save_volume(next_volume);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "volume remains %u%% because persistence failed",
                 runtime->controller->volume_percent);
        return;
    }

    update_shared_volume(runtime->controller, next_volume);
    const int32_t target_gain = volume_to_gain_q15(next_volume);
    ramp_begin(&runtime->volume_ramp,
               runtime->volume_gain_q15,
               target_gain,
               frames_for_ms(runtime, SC_PLAYBACK_VOLUME_RAMP_MS));
    ESP_LOGI(TAG, "volume=%u%% target_gain_q15=%" PRId32 " ramp=%ums persisted=yes",
             next_volume, target_gain, SC_PLAYBACK_VOLUME_RAMP_MS);
}

static void update_debug_selection(playback_runtime_t *runtime,
                                   sc_audio_source_t source,
                                   sc_audio_tone_note_t note,
                                   sc_audio_i2s_packing_t packing)
{
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    const bool audible_change =
        source != runtime->active_source ||
        packing != runtime->active_i2s_packing ||
        (source == SC_AUDIO_SOURCE_TONE &&
         runtime->active_source == SC_AUDIO_SOURCE_TONE &&
         note != runtime->active_tone_note);

    runtime->requested_source = source;
    runtime->requested_tone_note = note;
    runtime->requested_i2s_packing = packing;
    if (!audible_change) {
        if (note != runtime->active_tone_note) {
            runtime->active_tone_note = note;
            update_shared_debug_config(runtime->controller,
                                       runtime->active_source,
                                       note,
                                       runtime->active_i2s_packing);
        }
        return;
    }
    if (runtime->controller->state == SC_PLAYBACK_STOPPED) {
        const esp_err_t result = apply_debug_profile(runtime, source, note, packing);
        if (result != ESP_OK) {
            runtime->requested_source = runtime->active_source;
            runtime->requested_tone_note = runtime->active_tone_note;
            runtime->requested_i2s_packing = runtime->active_i2s_packing;
            ESP_LOGE(TAG, "debug selection unchanged: %s",
                     esp_err_to_name(result));
        }
        return;
    }
    if (audible_change &&
        (runtime->controller->state == SC_PLAYBACK_STARTING ||
         runtime->controller->state == SC_PLAYBACK_PLAYING)) {
        runtime->source_switch_pending = true;
        ramp_begin(&runtime->playback_ramp,
                   runtime->playback_gain_q15, 0,
                   frames_for_ms(runtime,
                                 SC_PLAYBACK_SOURCE_SWITCH_FADE_MS));
        runtime->source_switch_fading_in = false;
    }
#else
    (void)runtime;
    (void)source;
    (void)note;
    (void)packing;
#endif
}

static void cycle_volume(playback_runtime_t *runtime)
{
    static const uint8_t LEVELS[] = {25U, 50U, 75U, 100U};
    uint8_t next_volume = LEVELS[0];
    for (size_t i = 0; i < sizeof(LEVELS); ++i) {
        if (runtime->controller->volume_percent < LEVELS[i]) {
            next_volume = LEVELS[i];
            break;
        }
    }

    set_volume(runtime, next_volume);
}

static void handle_command(playback_runtime_t *runtime,
                           const sc_playback_command_t *command)
{
    if (runtime->controller->state == SC_PLAYBACK_FAULT) {
        ESP_LOGW(TAG, "command ignored in FAULT state");
        return;
    }

    switch (command->type) {
    case SC_PLAYBACK_COMMAND_TOGGLE:
        if (runtime->controller->state == SC_PLAYBACK_STOPPED) {
            begin_playback(runtime, SC_SESSION_DURATION_SECONDS);
        } else if (runtime->controller->state == SC_PLAYBACK_STARTING ||
                   runtime->controller->state == SC_PLAYBACK_PLAYING) {
            begin_stop(runtime, SC_PLAYBACK_MANUAL_STOP_FADE_MS, "manual",
                       SC_PLAYBACK_STOP_REASON_MANUAL);
        } else {
            ESP_LOGW(TAG, "toggle ignored while state=%s",
                     sc_playback_state_name(runtime->controller->state));
        }
        break;
    case SC_PLAYBACK_COMMAND_CYCLE_VOLUME:
        cycle_volume(runtime);
        break;
    case SC_PLAYBACK_COMMAND_PLAY:
        if (runtime->controller->state == SC_PLAYBACK_STOPPED) {
            begin_playback(runtime, command->duration_seconds);
        } else {
            ESP_LOGW(TAG, "PLAY ignored while state=%s",
                     sc_playback_state_name(runtime->controller->state));
        }
        break;
    case SC_PLAYBACK_COMMAND_STOP:
        if (runtime->controller->state == SC_PLAYBACK_STARTING ||
            runtime->controller->state == SC_PLAYBACK_PLAYING) {
            begin_stop(runtime, SC_PLAYBACK_MANUAL_STOP_FADE_MS, "remote",
                       SC_PLAYBACK_STOP_REASON_MANUAL);
        }
        break;
    case SC_PLAYBACK_COMMAND_SET_VOLUME:
        set_volume(runtime, command->volume_percent);
        break;
    case SC_PLAYBACK_COMMAND_SET_SOURCE:
        update_debug_selection(runtime, command->source,
                               runtime->requested_tone_note,
                               runtime->requested_i2s_packing);
        break;
    case SC_PLAYBACK_COMMAND_SET_TONE_NOTE:
        update_debug_selection(runtime, runtime->requested_source,
                               command->tone_note,
                               runtime->requested_i2s_packing);
        break;
    case SC_PLAYBACK_COMMAND_SET_I2S_PACKING:
        update_debug_selection(runtime, runtime->requested_source,
                               runtime->requested_tone_note,
                               command->i2s_packing);
        break;
    default:
        ESP_LOGW(TAG, "unknown standalone command=%d", (int)command->type);
        break;
    }
}

static void drain_commands(playback_runtime_t *runtime)
{
    sc_playback_command_t command;
    while (xQueueReceive(runtime->controller->command_queue, &command, 0) == pdTRUE) {
        handle_command(runtime, &command);
    }
}

static esp_err_t write_playback_block(playback_runtime_t *runtime)
{
    int32_t mono[SC_PLAYBACK_BLOCK_FRAMES];

    esp_err_t result = fill_mono_block(runtime, mono, SC_PLAYBACK_BLOCK_FRAMES);
    if (result != ESP_OK) {
        return result;
    }

    for (size_t i = 0; i < SC_PLAYBACK_BLOCK_FRAMES; ++i) {
        runtime->playback_gain_q15 = ramp_next(&runtime->playback_ramp);
        runtime->volume_gain_q15 = ramp_next(&runtime->volume_ramp);
        const int64_t scaled =
            ((int64_t)mono[i] * runtime->playback_gain_q15 *
             runtime->volume_gain_q15) >> 30;
        mono[i] = (int32_t)scaled;
    }

    result = sc_audio_output_write_mono_q31(runtime->controller->output,
                                            mono,
                                            SC_PLAYBACK_BLOCK_FRAMES);
    if (result == ESP_OK) {
        runtime->session_frames += SC_PLAYBACK_BLOCK_FRAMES;
    }
    return result;
}

static esp_err_t write_trailing_silence(playback_runtime_t *runtime)
{
    const int32_t silence[SC_PLAYBACK_BLOCK_FRAMES] = {0};
    return sc_audio_output_write_mono_q31(runtime->controller->output,
                                          silence,
                                          SC_PLAYBACK_BLOCK_FRAMES);
}

static void update_timing_and_state(playback_runtime_t *runtime)
{
    const int64_t now_us = esp_timer_get_time();
    const int64_t fade_start_us = runtime->session_deadline_us -
        ((int64_t)SC_PLAYBACK_TIMEOUT_FADE_MS * 1000LL);

    if (!runtime->timeout_fade_started && now_us >= fade_start_us &&
        (runtime->controller->state == SC_PLAYBACK_STARTING ||
         runtime->controller->state == SC_PLAYBACK_PLAYING)) {
        runtime->timeout_fade_started = true;
        begin_stop(runtime, SC_PLAYBACK_TIMEOUT_FADE_MS, "session-timeout",
                   SC_PLAYBACK_STOP_REASON_TIMER_EXPIRED);
    }

#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    if (runtime->source_switch_pending &&
        !runtime->source_switch_fading_in &&
        ramp_complete(&runtime->playback_ramp) &&
        (runtime->controller->state == SC_PLAYBACK_STARTING ||
         runtime->controller->state == SC_PLAYBACK_PLAYING)) {
        const esp_err_t result = apply_debug_profile(
            runtime,
            runtime->requested_source,
            runtime->requested_tone_note,
            runtime->requested_i2s_packing);
        if (result != ESP_OK) {
            runtime->requested_source = runtime->active_source;
            runtime->requested_tone_note = runtime->active_tone_note;
            runtime->requested_i2s_packing = runtime->active_i2s_packing;
            ESP_LOGE(TAG, "source switch rejected; restoring prior profile: %s",
                     esp_err_to_name(result));
        }
        runtime->source_switch_fading_in = true;
        ramp_begin(&runtime->playback_ramp, 0, INT16_MAX,
                   frames_for_ms(runtime,
                                 SC_PLAYBACK_SOURCE_SWITCH_FADE_MS));
    } else if (runtime->source_switch_pending &&
               runtime->source_switch_fading_in &&
               ramp_complete(&runtime->playback_ramp)) {
        runtime->source_switch_pending = false;
        runtime->source_switch_fading_in = false;
    }
#endif

    if (runtime->controller->state == SC_PLAYBACK_STARTING &&
        ramp_complete(&runtime->playback_ramp)) {
        update_shared_status(runtime->controller, SC_PLAYBACK_PLAYING,
                             runtime->stop_reason,
                             runtime->session_deadline_us, 0U);
        ESP_LOGI(TAG, "state=PLAYING");
    }

    if (runtime->controller->state == SC_PLAYBACK_STOPPING &&
        ramp_complete(&runtime->playback_ramp)) {
        const esp_err_t result = write_trailing_silence(runtime);
        if (result != ESP_OK) {
            enter_fault(runtime, "trailing silence write", result);
            return;
        }
        update_shared_status(runtime->controller, SC_PLAYBACK_STOPPED,
                             runtime->stop_reason, 0, 0U);
        ESP_LOGI(TAG,
                 "state=STOPPED reason=%s frames=%" PRIu64 " bytes=%" PRIu64
                 " loops=%" PRIu32,
                 runtime->stop_reason == SC_PLAYBACK_STOP_REASON_TIMER_EXPIRED
                     ? "session-timeout" : "manual",
                 runtime->session_frames,
                 runtime->session_frames *
                     sc_audio_output_bytes_per_frame(runtime->controller->output),
                 runtime->loop_count);
        return;
    }

    if (now_us >= runtime->next_heartbeat_us &&
        runtime->controller->state != SC_PLAYBACK_STOPPED) {
        int64_t remaining_seconds =
            (runtime->session_deadline_us - now_us + 999999LL) / 1000000LL;
        if (remaining_seconds < 0) {
            remaining_seconds = 0;
        }
        ESP_LOGI(TAG,
                 "heartbeat state=%s frames=%" PRIu64 " bytes=%" PRIu64
                 " loops=%" PRIu32 " remaining=%" PRId64 "s",
                 sc_playback_state_name(runtime->controller->state),
                 runtime->session_frames,
                 runtime->session_frames *
                     sc_audio_output_bytes_per_frame(runtime->controller->output),
                 runtime->loop_count,
                 remaining_seconds);
        runtime->next_heartbeat_us +=
            ((int64_t)SC_PLAYBACK_HEARTBEAT_SECONDS * 1000000LL);
    }
}

static void playback_task(void *argument)
{
    playback_runtime_t runtime = {
        .controller = argument,
        .playback_gain_q15 = 0,
        .volume_gain_q15 = 0,
        .active_source = SC_AUDIO_SOURCE_MUSIC,
        .active_tone_note = SC_AUDIO_TONE_C5,
        .active_i2s_packing = SC_AUDIO_I2S_PACKING_NATIVE,
        .requested_source = SC_AUDIO_SOURCE_MUSIC,
        .requested_tone_note = SC_AUDIO_TONE_C5,
        .requested_i2s_packing = SC_AUDIO_I2S_PACKING_NATIVE,
        .active_sample_rate_hz = SC_AUDIO_SAMPLE_RATE_HZ,
        .active_data_bits = SC_AUDIO_BITS_PER_SAMPLE,
    };
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    sc_tone_generator_init(&runtime.tone_generator, SC_AUDIO_TONE_C5);
#endif

    for (;;) {
        if (runtime.controller->state == SC_PLAYBACK_STOPPED ||
            runtime.controller->state == SC_PLAYBACK_FAULT) {
            sc_playback_command_t command;
            if (xQueueReceive(runtime.controller->command_queue,
                              &command,
                              portMAX_DELAY) == pdTRUE) {
                handle_command(&runtime, &command);
            }
            continue;
        }

        drain_commands(&runtime);
        if (runtime.controller->state == SC_PLAYBACK_FAULT) {
            continue;
        }

        const esp_err_t result = write_playback_block(&runtime);
        if (result != ESP_OK) {
            enter_fault(&runtime, "stored-audio pipeline", result);
            continue;
        }
        update_timing_and_state(&runtime);
    }
}

esp_err_t sc_playback_controller_init(sc_playback_controller_t *controller,
                                      sc_audio_output_t *output,
                                      sc_wav_reader_t *reader,
                                      uint8_t initial_volume_percent)
{
    ESP_RETURN_ON_FALSE(controller != NULL && output != NULL && reader != NULL,
                        ESP_ERR_INVALID_ARG, TAG, "invalid controller arguments");
    ESP_RETURN_ON_FALSE(output->initialized && reader->file != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "dependencies are not ready");
    ESP_RETURN_ON_FALSE(initial_volume_percent <= SC_VOLUME_MAX_PERCENT,
                        ESP_ERR_INVALID_ARG, TAG, "initial volume is out of range");

    memset(controller, 0, sizeof(*controller));
    controller->output = output;
    controller->reader = reader;
    controller->state = SC_PLAYBACK_STOPPED;
    controller->volume_percent = initial_volume_percent;
    controller->source = SC_AUDIO_SOURCE_MUSIC;
    controller->tone_note = SC_AUDIO_TONE_C5;
    controller->i2s_packing = SC_AUDIO_I2S_PACKING_NATIVE;
    controller->stop_reason = SC_PLAYBACK_STOP_REASON_BOOT;
    controller->status_mutex = xSemaphoreCreateMutex();
    if (controller->status_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    controller->command_queue = xQueueCreate(4, sizeof(sc_playback_command_t));
    if (controller->command_queue == NULL) {
        vSemaphoreDelete(controller->status_mutex);
        controller->status_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }

    const BaseType_t created = xTaskCreate(playback_task,
                                           "audio_playback",
                                           8192,
                                           controller,
                                           8,
                                           &controller->task);
    if (created != pdPASS) {
        vQueueDelete(controller->command_queue);
        controller->command_queue = NULL;
        vSemaphoreDelete(controller->status_mutex);
        controller->status_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG,
             "controller ready: state=STOPPED volume=%u%% session=%us "
             "start_fade=%ums stop_fade=%ums timeout_fade=%ums",
             controller->volume_percent,
             SC_SESSION_DURATION_SECONDS,
             SC_PLAYBACK_START_FADE_MS,
             SC_PLAYBACK_MANUAL_STOP_FADE_MS,
             SC_PLAYBACK_TIMEOUT_FADE_MS);
    return ESP_OK;
}

esp_err_t sc_playback_controller_send(sc_playback_controller_t *controller,
                                      sc_playback_command_type_t command)
{
    ESP_RETURN_ON_FALSE(controller != NULL && controller->command_queue != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "controller is not initialized");
    const sc_playback_command_t message = {.type = command};
    if (xQueueSend(controller->command_queue, &message, 0) != pdTRUE) {
        ESP_LOGE(TAG, "command queue full; command=%d rejected", (int)command);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t send_message(sc_playback_controller_t *controller,
                              const sc_playback_command_t *message)
{
    ESP_RETURN_ON_FALSE(controller != NULL && message != NULL &&
                            controller->command_queue != NULL,
                        ESP_ERR_INVALID_STATE, TAG,
                        "controller is not initialized");
    if (xQueueSend(controller->command_queue, message, 0) != pdTRUE) {
        ESP_LOGE(TAG, "command queue full; command=%d rejected",
                 (int)message->type);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t sc_playback_controller_play(sc_playback_controller_t *controller,
                                      uint32_t duration_seconds)
{
    ESP_RETURN_ON_FALSE(duration_seconds >= SC_SESSION_DURATION_MIN_SECONDS &&
                            duration_seconds <= SC_SESSION_DURATION_MAX_SECONDS,
                        ESP_ERR_INVALID_ARG, TAG, "invalid session duration");
    const sc_playback_command_t message = {
        .type = SC_PLAYBACK_COMMAND_PLAY,
        .duration_seconds = duration_seconds,
    };
    return send_message(controller, &message);
}

esp_err_t sc_playback_controller_stop(sc_playback_controller_t *controller)
{
    const sc_playback_command_t message = {.type = SC_PLAYBACK_COMMAND_STOP};
    return send_message(controller, &message);
}

esp_err_t sc_playback_controller_set_volume(sc_playback_controller_t *controller,
                                            uint8_t volume_percent)
{
    ESP_RETURN_ON_FALSE(volume_percent <= SC_VOLUME_MAX_PERCENT,
                        ESP_ERR_INVALID_ARG, TAG, "invalid volume");
    const sc_playback_command_t message = {
        .type = SC_PLAYBACK_COMMAND_SET_VOLUME,
        .volume_percent = volume_percent,
    };
    return send_message(controller, &message);
}

esp_err_t sc_playback_controller_set_source(sc_playback_controller_t *controller,
                                            sc_audio_source_t source)
{
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    ESP_RETURN_ON_FALSE(source < SC_AUDIO_SOURCE_COUNT,
                        ESP_ERR_INVALID_ARG, TAG, "invalid audio source");
    const sc_playback_command_t message = {
        .type = SC_PLAYBACK_COMMAND_SET_SOURCE,
        .source = source,
    };
    return send_message(controller, &message);
#else
    (void)controller;
    (void)source;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t sc_playback_controller_set_i2s_packing(
    sc_playback_controller_t *controller,
    sc_audio_i2s_packing_t packing)
{
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    ESP_RETURN_ON_FALSE(packing < SC_AUDIO_I2S_PACKING_COUNT,
                        ESP_ERR_INVALID_ARG, TAG, "invalid I2S packing");
    const sc_playback_command_t message = {
        .type = SC_PLAYBACK_COMMAND_SET_I2S_PACKING,
        .i2s_packing = packing,
    };
    return send_message(controller, &message);
#else
    (void)controller;
    (void)packing;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t sc_playback_controller_set_tone_note(sc_playback_controller_t *controller,
                                               sc_audio_tone_note_t note)
{
#if CONFIG_SC_ENABLE_TOUCH_DEBUG_AUDIO
    ESP_RETURN_ON_FALSE(note < SC_AUDIO_TONE_COUNT,
                        ESP_ERR_INVALID_ARG, TAG, "invalid tone note");
    const sc_playback_command_t message = {
        .type = SC_PLAYBACK_COMMAND_SET_TONE_NOTE,
        .tone_note = note,
    };
    return send_message(controller, &message);
#else
    (void)controller;
    (void)note;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t sc_playback_controller_get_snapshot(sc_playback_controller_t *controller,
                                              sc_playback_snapshot_t *snapshot)
{
    ESP_RETURN_ON_FALSE(controller != NULL && snapshot != NULL &&
                            controller->status_mutex != NULL,
                        ESP_ERR_INVALID_STATE, TAG,
                        "controller is not initialized");
    if (xSemaphoreTake(controller->status_mutex, pdMS_TO_TICKS(20)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    snapshot->state = controller->state;
    snapshot->volume_percent = controller->volume_percent;
    snapshot->source = controller->source;
    snapshot->tone_note = controller->tone_note;
    snapshot->i2s_packing = controller->i2s_packing;
    snapshot->stop_reason = controller->stop_reason;
    snapshot->fault_code = controller->fault_code;
    const int64_t deadline_us = controller->session_deadline_us;
    xSemaphoreGive(controller->status_mutex);

    snapshot->remaining_seconds = 0U;
    if (deadline_us > 0 && snapshot->state != SC_PLAYBACK_STOPPED &&
        snapshot->state != SC_PLAYBACK_FAULT) {
        const int64_t remaining_us = deadline_us - esp_timer_get_time();
        if (remaining_us > 0) {
            snapshot->remaining_seconds =
                (uint32_t)((remaining_us + 999999LL) / 1000000LL);
        }
    }
    return ESP_OK;
}
