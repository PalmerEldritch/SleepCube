#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "sc_audio_protocol.h"

esp_err_t sc_audio_service_start(void);

/** Request playback on the selected backend; remote enable is always finite. */
esp_err_t sc_audio_service_set_playback(bool enable);

/** Toggle from the latest authoritative playback state. */
esp_err_t sc_audio_service_toggle_playback(void);

/** Apply a relative five-percent volume step without blocking the caller. */
esp_err_t sc_audio_service_change_volume(int delta_steps);

/** Set absolute 0..100 percent volume; the remote backend coalesces requests. */
esp_err_t sc_audio_service_set_volume_percent(uint8_t percent);

/** Return playback state confirmed by the selected backend. */
bool sc_audio_service_get_playback_enabled(void);

/** Return the latest volume confirmed by the selected backend. */
uint8_t sc_audio_service_get_volume_percent(void);

/** Return true only when audio accepts normal application commands. */
bool sc_audio_service_is_available(void);

/** Return true only after authoritative remote state reconciliation. */
bool sc_audio_service_is_synchronized(void);

/** Development-image source controls. They require the matching Audio image. */
bool sc_audio_service_has_debug_source(void);
sc_audio_source_t sc_audio_service_get_source(void);
sc_audio_tone_note_t sc_audio_service_get_tone_note(void);
esp_err_t sc_audio_service_set_source(sc_audio_source_t source);
esp_err_t sc_audio_service_set_tone_note(sc_audio_tone_note_t note);
/** Restore music/C5 now, or after the matched Audio image reconnects. */
void sc_audio_service_restore_debug_defaults(void);
