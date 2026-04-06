#include "audio_diag_console.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "audio_player.h"

static const char *TAG = "sc_audio_diag_cli";

#define SC_AUDIO_DIAG_CONSOLE_STACK_WORDS  (6144)
#define SC_AUDIO_DIAG_CONSOLE_LINE_LEN     (128)
static char s_track_list_buf[SC_AUDIO_FS_MAX_TRACKS][SC_AUDIO_FS_MAX_PATH_LEN];
static char s_track_entry_buf[SC_AUDIO_FS_MAX_TRACKS][SC_AUDIO_FS_MAX_PATH_LEN];
static char s_track_path_buf[SC_AUDIO_FS_MAX_PATH_LEN];

static bool sc_audio_diag_parse_u16(const char *text, uint16_t *value)
{
    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if ((text == NULL) || (value == NULL) || (end == text) || (*end != '\0') || (parsed < 0) || (parsed > 65535L)) {
        return false;
    }
    *value = (uint16_t)parsed;
    return true;
}

static bool sc_audio_diag_parse_u8(const char *text, uint8_t *value)
{
    uint16_t parsed = 0;
    if (!sc_audio_diag_parse_u16(text, &parsed) || (parsed > 255U)) {
        return false;
    }
    *value = (uint8_t)parsed;
    return true;
}

static void sc_audio_diag_list_tracks(void)
{
    size_t count = sc_audio_fs_list_sd_tracks(s_track_list_buf, SC_AUDIO_FS_MAX_TRACKS);
    if (count == 0U) {
        size_t entry_count = sc_audio_fs_list_sd_entries(s_track_entry_buf, SC_AUDIO_FS_MAX_TRACKS);
        ESP_LOGI(TAG, "tracks: no SD MP3 files found");
        if (entry_count == 0U) {
            ESP_LOGI(TAG, "tracks: no visible root entries found on /sdcard");
        } else {
            ESP_LOGI(TAG, "tracks: visible root entries on /sdcard:");
            for (size_t i = 0; i < entry_count; i++) {
                ESP_LOGI(TAG, "entry %u: %s", (unsigned)(i + 1U), s_track_entry_buf[i]);
            }
        }
        return;
    }

    for (size_t i = 0; i < count; i++) {
        ESP_LOGI(TAG, "track %u: %s", (unsigned)(i + 1U), s_track_list_buf[i]);
    }
}

static bool sc_audio_diag_select_track(const char *selector)
{
    uint16_t index = 0;

    if (sc_audio_diag_parse_u16(selector, &index) && (index > 0U)) {
        size_t count = sc_audio_fs_list_sd_tracks(s_track_list_buf, SC_AUDIO_FS_MAX_TRACKS);
        if ((size_t)index > count) {
            return false;
        }
        return sc_audio_player_set_mp3_path(s_track_list_buf[index - 1U]);
    }

    if (!sc_audio_fs_resolve_sd_track(selector, s_track_path_buf, sizeof(s_track_path_buf))) {
        return false;
    }
    return sc_audio_player_set_mp3_path(s_track_path_buf);
}

static void sc_audio_diag_print_help(void)
{
    ESP_LOGI(TAG, "commands: help, status, source <mp3|tone|sweep>, tone <hz>, amp <pct>,");
    ESP_LOGI(TAG, "          sweep <start_hz> <end_hz> <period_ms>, hpf <cutoff_hz> [stages],");
    ESP_LOGI(TAG, "          stages <count>, vol <pct>, play <on|off|toggle|stop>,");
    ESP_LOGI(TAG, "          mp3src <auto|spiffs|sd>, mp3mix <stereo|mono|left|right>, mp3gain <db>,");
    ESP_LOGI(TAG, "          mp3limit <on|off> [threshold_pct], mp3eq <off|lpf|presence> [hz] [depth],");
    ESP_LOGI(TAG, "          mp3stats <show|reset>,");
    ESP_LOGI(TAG, "          sdremount, tracks,");
    ESP_LOGI(TAG, "          track <index|filename|/sdcard/file.mp3>");
}

static void sc_audio_diag_print_status(void)
{
    sc_audio_runtime_config_t cfg = { 0 };
    sc_audio_player_get_runtime_config(&cfg);
    ESP_LOGI(TAG,
             "status: play=%s source=%s mp3src=%s mp3mix=%s mp3gain=-%udB mp3limit=%s@%u%% mp3eq=%s/%uHz/%u%% vol=%u%% tone=%uHz amp=%u%% sweep=%u-%uHz/%ums hpf=%uHz x%u mp3(dec=%lu sync=%lu rate=%lu)",
             cfg.playback_enabled ? "on" : "off",
             (cfg.source_mode == SC_AUDIO_SOURCE_MODE_MP3) ? "mp3" :
             (cfg.source_mode == SC_AUDIO_SOURCE_MODE_SWEEP) ? "sweep" : "tone",
             (cfg.mp3_source == SC_AUDIO_MP3_SOURCE_SPIFFS) ? "spiffs" :
             (cfg.mp3_source == SC_AUDIO_MP3_SOURCE_SD) ? "sd" : "auto",
             (cfg.mp3_mix_mode == SC_AUDIO_MP3_MIX_MONO) ? "mono" :
             (cfg.mp3_mix_mode == SC_AUDIO_MP3_MIX_LEFT) ? "left" :
             (cfg.mp3_mix_mode == SC_AUDIO_MP3_MIX_RIGHT) ? "right" : "stereo",
             (unsigned)cfg.mp3_pre_gain_db,
             cfg.mp3_limiter_enabled ? "on" : "off",
             (unsigned)cfg.mp3_limiter_threshold_pct,
             (cfg.mp3_eq_mode == SC_AUDIO_MP3_EQ_LPF) ? "lpf" :
             (cfg.mp3_eq_mode == SC_AUDIO_MP3_EQ_PRESENCE_CUT) ? "presence" : "off",
             (unsigned)cfg.mp3_eq_cutoff_hz,
             (unsigned)cfg.mp3_eq_depth_pct,
             (unsigned)cfg.volume_percent,
             (unsigned)cfg.tone_frequency_hz,
             (unsigned)cfg.tone_amplitude_percent,
             (unsigned)cfg.sweep_start_hz,
             (unsigned)cfg.sweep_end_hz,
             (unsigned)cfg.sweep_period_ms,
             (unsigned)cfg.hpf_cutoff_hz,
             (unsigned)cfg.hpf_stages,
             (unsigned long)cfg.mp3_decode_error_count,
             (unsigned long)cfg.mp3_sync_miss_count,
             (unsigned long)cfg.mp3_rate_mismatch_count);
    ESP_LOGI(TAG, "mp3 paths: auto=%s spiffs=%s sd=%s selected=%s",
             sc_audio_fs_mp3_source_available(SC_AUDIO_MP3_SOURCE_AUTO) ? "ok" : "missing",
             sc_audio_fs_mp3_source_available(SC_AUDIO_MP3_SOURCE_SPIFFS) ? "ok" : "missing",
             sc_audio_fs_mp3_source_available(SC_AUDIO_MP3_SOURCE_SD) ? "ok" : "missing",
             cfg.mp3_path);
}

static void sc_audio_diag_handle_line(char *line)
{
    char *argv[5] = { 0 };
    int argc = 0;
    char *token = strtok(line, " \t\r\n");
    while ((token != NULL) && (argc < 5)) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    if (argc == 0) {
        return;
    }

    if (strcmp(argv[0], "help") == 0) {
        sc_audio_diag_print_help();
        return;
    }

    if (strcmp(argv[0], "status") == 0) {
        sc_audio_diag_print_status();
        return;
    }

    if (strcmp(argv[0], "tracks") == 0) {
        sc_audio_diag_list_tracks();
        return;
    }

    if (strcmp(argv[0], "sdremount") == 0) {
        esp_err_t err = sc_audio_fs_remount_sdcard();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "sdremount failed: %s", esp_err_to_name(err));
        }
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "source") == 0) && (argc >= 2)) {
        if (strcmp(argv[1], "mp3") == 0) {
            sc_audio_player_set_source_mode(SC_AUDIO_SOURCE_MODE_MP3);
        } else if (strcmp(argv[1], "tone") == 0) {
            sc_audio_player_set_source_mode(SC_AUDIO_SOURCE_MODE_TONE);
        } else if (strcmp(argv[1], "sweep") == 0) {
            sc_audio_player_set_source_mode(SC_AUDIO_SOURCE_MODE_SWEEP);
        } else {
            ESP_LOGW(TAG, "unknown source: %s", argv[1]);
            return;
        }
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "tone") == 0) && (argc >= 2)) {
        uint16_t hz = 0;
        if (!sc_audio_diag_parse_u16(argv[1], &hz) || (hz == 0U)) {
            ESP_LOGW(TAG, "usage: tone <hz>");
            return;
        }
        sc_audio_player_set_tone_frequency_hz(hz);
        sc_audio_player_set_source_mode(SC_AUDIO_SOURCE_MODE_TONE);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "amp") == 0) && (argc >= 2)) {
        uint8_t pct = 0;
        if (!sc_audio_diag_parse_u8(argv[1], &pct) || (pct > 100U) || (pct == 0U)) {
            ESP_LOGW(TAG, "usage: amp <1-100>");
            return;
        }
        sc_audio_player_set_tone_amplitude_percent(pct);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "sweep") == 0) && (argc >= 4)) {
        uint16_t start_hz = 0;
        uint16_t end_hz = 0;
        uint16_t period_ms = 0;
        if (!sc_audio_diag_parse_u16(argv[1], &start_hz) ||
            !sc_audio_diag_parse_u16(argv[2], &end_hz) ||
            !sc_audio_diag_parse_u16(argv[3], &period_ms) ||
            (start_hz == 0U) || (end_hz == 0U) || (period_ms == 0U)) {
            ESP_LOGW(TAG, "usage: sweep <start_hz> <end_hz> <period_ms>");
            return;
        }
        sc_audio_player_set_sweep(start_hz, end_hz, period_ms);
        sc_audio_player_set_source_mode(SC_AUDIO_SOURCE_MODE_SWEEP);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "hpf") == 0) && (argc >= 2)) {
        uint16_t cutoff_hz = 0;
        uint8_t stages = sc_audio_player_get_hpf_stages();
        if (!sc_audio_diag_parse_u16(argv[1], &cutoff_hz)) {
            ESP_LOGW(TAG, "usage: hpf <cutoff_hz> [stages]");
            return;
        }
        if (argc >= 3) {
            if (!sc_audio_diag_parse_u8(argv[2], &stages)) {
                ESP_LOGW(TAG, "usage: hpf <cutoff_hz> [stages]");
                return;
            }
        }
        sc_audio_player_set_hpf(cutoff_hz, stages);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "stages") == 0) && (argc >= 2)) {
        uint8_t stages = 0;
        if (!sc_audio_diag_parse_u8(argv[1], &stages)) {
            ESP_LOGW(TAG, "usage: stages <count>");
            return;
        }
        sc_audio_player_set_hpf(sc_audio_player_get_hpf_cutoff_hz(), stages);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "vol") == 0) && (argc >= 2)) {
        uint8_t pct = 0;
        if (!sc_audio_diag_parse_u8(argv[1], &pct) || (pct > 100U)) {
            ESP_LOGW(TAG, "usage: vol <0-100>");
            return;
        }
        sc_audio_player_set_volume_percent(pct);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "play") == 0) && (argc >= 2)) {
        if (strcmp(argv[1], "on") == 0) {
            sc_audio_player_set_enabled(true);
        } else if (strcmp(argv[1], "off") == 0) {
            sc_audio_player_set_enabled(false);
        } else if (strcmp(argv[1], "toggle") == 0) {
            sc_audio_player_set_enabled(!sc_audio_player_get_enabled());
        } else if (strcmp(argv[1], "stop") == 0) {
            sc_audio_player_request_stop();
        } else {
            ESP_LOGW(TAG, "usage: play <on|off|toggle|stop>");
            return;
        }
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "mp3src") == 0) && (argc >= 2)) {
        if (strcmp(argv[1], "auto") == 0) {
            sc_audio_player_set_mp3_source(SC_AUDIO_MP3_SOURCE_AUTO);
        } else if (strcmp(argv[1], "spiffs") == 0) {
            sc_audio_player_set_mp3_source(SC_AUDIO_MP3_SOURCE_SPIFFS);
        } else if (strcmp(argv[1], "sd") == 0) {
            sc_audio_player_set_mp3_source(SC_AUDIO_MP3_SOURCE_SD);
        } else {
            ESP_LOGW(TAG, "usage: mp3src <auto|spiffs|sd>");
            return;
        }
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "mp3mix") == 0) && (argc >= 2)) {
        if (strcmp(argv[1], "stereo") == 0) {
            sc_audio_player_set_mp3_mix_mode(SC_AUDIO_MP3_MIX_STEREO);
        } else if (strcmp(argv[1], "mono") == 0) {
            sc_audio_player_set_mp3_mix_mode(SC_AUDIO_MP3_MIX_MONO);
        } else if (strcmp(argv[1], "left") == 0) {
            sc_audio_player_set_mp3_mix_mode(SC_AUDIO_MP3_MIX_LEFT);
        } else if (strcmp(argv[1], "right") == 0) {
            sc_audio_player_set_mp3_mix_mode(SC_AUDIO_MP3_MIX_RIGHT);
        } else {
            ESP_LOGW(TAG, "usage: mp3mix <stereo|mono|left|right>");
            return;
        }
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "mp3gain") == 0) && (argc >= 2)) {
        uint8_t db = 0;
        if (!sc_audio_diag_parse_u8(argv[1], &db) || (db > 24U)) {
            ESP_LOGW(TAG, "usage: mp3gain <0-24>");
            return;
        }
        sc_audio_player_set_mp3_pre_gain_db(db);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "mp3limit") == 0) && (argc >= 2)) {
        bool enable = false;
        uint8_t threshold_pct = sc_audio_player_get_mp3_limiter_threshold_pct();
        if (strcmp(argv[1], "on") == 0) {
            enable = true;
        } else if (strcmp(argv[1], "off") == 0) {
            enable = false;
        } else {
            ESP_LOGW(TAG, "usage: mp3limit <on|off> [50-99]");
            return;
        }

        if (argc >= 3) {
            if (!sc_audio_diag_parse_u8(argv[2], &threshold_pct) ||
                (threshold_pct < 50U) || (threshold_pct > 99U)) {
                ESP_LOGW(TAG, "usage: mp3limit <on|off> [50-99]");
                return;
            }
        }

        sc_audio_player_set_mp3_limiter(enable, threshold_pct);
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "mp3eq") == 0) && (argc >= 2)) {
        sc_audio_mp3_eq_mode_t mode = SC_AUDIO_MP3_EQ_OFF;
        uint16_t cutoff_hz = sc_audio_player_get_mp3_eq_cutoff_hz();
        uint8_t depth_pct = sc_audio_player_get_mp3_eq_depth_pct();

        if (strcmp(argv[1], "off") == 0) {
            mode = SC_AUDIO_MP3_EQ_OFF;
        } else if (strcmp(argv[1], "lpf") == 0) {
            mode = SC_AUDIO_MP3_EQ_LPF;
        } else if (strcmp(argv[1], "presence") == 0) {
            mode = SC_AUDIO_MP3_EQ_PRESENCE_CUT;
        } else {
            ESP_LOGW(TAG, "usage: mp3eq <off|lpf|presence> [hz] [depth]");
            return;
        }

        if ((argc >= 3) && !sc_audio_diag_parse_u16(argv[2], &cutoff_hz)) {
            ESP_LOGW(TAG, "usage: mp3eq <off|lpf|presence> [hz] [depth]");
            return;
        }
        if ((argc >= 4) && (!sc_audio_diag_parse_u8(argv[3], &depth_pct) || (depth_pct > 100U))) {
            ESP_LOGW(TAG, "usage: mp3eq <off|lpf|presence> [hz] [depth]");
            return;
        }

        if (mode == SC_AUDIO_MP3_EQ_OFF) {
            sc_audio_player_set_mp3_eq(mode, cutoff_hz, depth_pct);
        } else if (mode == SC_AUDIO_MP3_EQ_LPF) {
            sc_audio_player_set_mp3_eq(mode, cutoff_hz, 100U);
        } else {
            sc_audio_player_set_mp3_eq(mode, cutoff_hz, depth_pct);
        }
        sc_audio_diag_print_status();
        return;
    }

    if ((strcmp(argv[0], "mp3stats") == 0) && (argc >= 2)) {
        if (strcmp(argv[1], "show") == 0) {
            sc_audio_diag_print_status();
        } else if (strcmp(argv[1], "reset") == 0) {
            sc_audio_player_reset_mp3_stats();
            sc_audio_diag_print_status();
        } else {
            ESP_LOGW(TAG, "usage: mp3stats <show|reset>");
        }
        return;
    }

    if ((strcmp(argv[0], "track") == 0) && (argc >= 2)) {
        if (!sc_audio_diag_select_track(argv[1])) {
            ESP_LOGW(TAG, "usage: track <index|filename|/sdcard/file.mp3>");
            return;
        }
        sc_audio_player_set_mp3_source(SC_AUDIO_MP3_SOURCE_SD);
        sc_audio_player_set_source_mode(SC_AUDIO_SOURCE_MODE_MP3);
        sc_audio_diag_print_status();
        return;
    }

    ESP_LOGW(TAG, "unknown command: %s", argv[0]);
    sc_audio_diag_print_help();
}

static void sc_audio_diag_console_task(void *arg)
{
    (void)arg;
    char line[SC_AUDIO_DIAG_CONSOLE_LINE_LEN];

    ESP_LOGI(TAG, "audio diagnostic serial console ready");
    sc_audio_diag_print_help();
    sc_audio_diag_print_status();

    while (1) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            vTaskDelay(pdMS_TO_TICKS(50));
            clearerr(stdin);
            continue;
        }
        sc_audio_diag_handle_line(line);
    }
}

esp_err_t sc_audio_diag_console_start(void)
{
    BaseType_t ok = xTaskCreate(sc_audio_diag_console_task,
                                "sc_audio_diag_cli",
                                SC_AUDIO_DIAG_CONSOLE_STACK_WORDS,
                                NULL,
                                3,
                                NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}
