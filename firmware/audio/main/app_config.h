#pragma once

#include <stdint.h>

#define SC_CONTENT_MOUNT_POINT             "/sdcard"
#define SC_CONTENT_DIRECTORY               SC_CONTENT_MOUNT_POINT "/sleepcube"
#define SC_CONTENT_FILE_PATH               SC_CONTENT_DIRECTORY "/audio.wav"

#define SC_CONTENT_SAMPLE_RATE_HZ           44100U
#define SC_CONTENT_BITS_PER_SAMPLE          16U
#define SC_CONTENT_CHANNEL_COUNT            1U

#define SC_PLAYBACK_BLOCK_FRAMES            512U
#define SC_PLAYBACK_START_FADE_MS           1000U
#define SC_PLAYBACK_MANUAL_STOP_FADE_MS     1000U
#define SC_PLAYBACK_TIMEOUT_FADE_MS         5000U
#define SC_PLAYBACK_VOLUME_RAMP_MS          250U
#define SC_PLAYBACK_HEARTBEAT_SECONDS       60U

#define SC_SESSION_DURATION_MIN_SECONDS     1U
#define SC_SESSION_DURATION_MAX_SECONDS     3600U

#define SC_VOLUME_MIN_PERCENT               0U
#define SC_VOLUME_MAX_PERCENT               100U
#define SC_VOLUME_DEFAULT_PERCENT           50U
#define SC_VOLUME_OUTPUT_CAP_Q15            8192U /* -12 dBFS at 100%. */

#define SC_SESSION_DURATION_SECONDS         3600U

#if CONFIG_SC_M2_ENDURANCE_BUILD
#define SC_BUILD_MODE_NAME                  "M2 60-minute endurance"
#else
#define SC_BUILD_MODE_NAME                  "M4 semantic link / 60-minute local default"
#endif

_Static_assert(SC_PLAYBACK_TIMEOUT_FADE_MS <
                   (SC_SESSION_DURATION_SECONDS * 1000U),
               "timeout fade must fit inside the finite session");
