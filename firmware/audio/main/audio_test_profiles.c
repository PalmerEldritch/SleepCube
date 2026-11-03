#include "audio_test_profiles.h"

#include <stddef.h>

#include "app_config.h"

#define SC_I2S_TEST_DIRECTORY SC_CONTENT_DIRECTORY "/i2s-test"

static const sc_audio_test_profile_t PROFILES[] = {
    {
        .source = SC_AUDIO_SOURCE_MUSIC,
        .path = SC_CONTENT_FILE_PATH,
        .sample_rate_hz = 44100U,
        .data_bits = 16U,
    },
    {
        .source = SC_AUDIO_SOURCE_TONE,
        .path = NULL,
        .sample_rate_hz = 44100U,
        .data_bits = 16U,
    },
    {
        .source = SC_AUDIO_SOURCE_TEST_44100_16,
        .path = SC_I2S_TEST_DIRECTORY "/ambient_44100_16bit_mono_12s.wav",
        .sample_rate_hz = 44100U,
        .data_bits = 16U,
    },
    {
        .source = SC_AUDIO_SOURCE_TEST_44100_24,
        .path = SC_I2S_TEST_DIRECTORY "/ambient_44100_24bit_mono_12s.wav",
        .sample_rate_hz = 44100U,
        .data_bits = 24U,
    },
    {
        .source = SC_AUDIO_SOURCE_TEST_48000_16,
        .path = SC_I2S_TEST_DIRECTORY "/ambient_48000_16bit_mono_12s.wav",
        .sample_rate_hz = 48000U,
        .data_bits = 16U,
    },
    {
        .source = SC_AUDIO_SOURCE_TEST_48000_24,
        .path = SC_I2S_TEST_DIRECTORY "/ambient_48000_24bit_mono_12s.wav",
        .sample_rate_hz = 48000U,
        .data_bits = 24U,
    },
    {
        .source = SC_AUDIO_SOURCE_TEST_32000_16,
        .path = SC_I2S_TEST_DIRECTORY "/ambient_32000_16bit_mono_12s.wav",
        .sample_rate_hz = 32000U,
        .data_bits = 16U,
    },
    {
        .source = SC_AUDIO_SOURCE_TEST_96000_16,
        .path = SC_I2S_TEST_DIRECTORY "/ambient_96000_16bit_mono_12s.wav",
        .sample_rate_hz = 96000U,
        .data_bits = 16U,
    },
};

bool sc_audio_test_profile_get(sc_audio_source_t source,
                               sc_audio_test_profile_t *profile)
{
    if (profile == NULL) {
        return false;
    }
    for (size_t i = 0; i < sizeof(PROFILES) / sizeof(PROFILES[0]); ++i) {
        if (PROFILES[i].source == source) {
            *profile = PROFILES[i];
            return true;
        }
    }
    return false;
}

