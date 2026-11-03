#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "audio_test_profiles.h"

typedef struct {
    sc_audio_source_t source;
    const char *path;
    uint32_t sample_rate_hz;
    uint8_t data_bits;
} expected_profile_t;

static void profiles_match_the_controlled_test_catalog(void)
{
    static const expected_profile_t expected[] = {
        {SC_AUDIO_SOURCE_MUSIC, "/sdcard/sleepcube/audio.wav", 44100U, 16U},
        {SC_AUDIO_SOURCE_TONE, NULL, 44100U, 16U},
        {SC_AUDIO_SOURCE_TEST_44100_16,
         "/sdcard/sleepcube/i2s-test/ambient_44100_16bit_mono_12s.wav",
         44100U, 16U},
        {SC_AUDIO_SOURCE_TEST_44100_24,
         "/sdcard/sleepcube/i2s-test/ambient_44100_24bit_mono_12s.wav",
         44100U, 24U},
        {SC_AUDIO_SOURCE_TEST_48000_16,
         "/sdcard/sleepcube/i2s-test/ambient_48000_16bit_mono_12s.wav",
         48000U, 16U},
        {SC_AUDIO_SOURCE_TEST_48000_24,
         "/sdcard/sleepcube/i2s-test/ambient_48000_24bit_mono_12s.wav",
         48000U, 24U},
        {SC_AUDIO_SOURCE_TEST_32000_16,
         "/sdcard/sleepcube/i2s-test/ambient_32000_16bit_mono_12s.wav",
         32000U, 16U},
        {SC_AUDIO_SOURCE_TEST_96000_16,
         "/sdcard/sleepcube/i2s-test/ambient_96000_16bit_mono_12s.wav",
         96000U, 16U},
    };

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        sc_audio_test_profile_t actual;
        assert(sc_audio_test_profile_get(expected[i].source, &actual));
        assert(actual.source == expected[i].source);
        assert(actual.sample_rate_hz == expected[i].sample_rate_hz);
        assert(actual.data_bits == expected[i].data_bits);
        if (expected[i].path == NULL) {
            assert(actual.path == NULL);
        } else {
            assert(actual.path != NULL);
            assert(strcmp(actual.path, expected[i].path) == 0);
        }
    }

    sc_audio_test_profile_t unused;
    assert(!sc_audio_test_profile_get(SC_AUDIO_SOURCE_COUNT, &unused));
    assert(!sc_audio_test_profile_get(SC_AUDIO_SOURCE_MUSIC, NULL));
}

int main(void)
{
    profiles_match_the_controlled_test_catalog();
    puts("audio_test_profiles tests passed");
    return 0;
}
