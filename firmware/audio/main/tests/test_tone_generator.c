#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tone_generator.h"

static void frequencies_and_octaves_are_correct(void)
{
    static const uint32_t expected[SC_AUDIO_TONE_COUNT] = {
        130813U, 261626U, 523251U, 1046502U, 2093005U, 4186009U,
    };
    sc_tone_generator_t generator;
    uint32_t previous_step = 0U;
    for (sc_audio_tone_note_t note = SC_AUDIO_TONE_C3;
         note < SC_AUDIO_TONE_COUNT;
         note = (sc_audio_tone_note_t)(note + 1)) {
        assert(sc_tone_generator_frequency_millihz(note) == expected[note]);
        sc_tone_generator_init(&generator, note);
        assert(generator.phase == 0U);
        assert(generator.phase_step > previous_step);
        if (previous_step != 0U) {
            const uint32_t doubled = previous_step * 2U;
            /* Published note frequencies are rounded to the nearest mHz. */
            assert(generator.phase_step >= doubled - 128U);
            assert(generator.phase_step <= doubled + 128U);
        }
        previous_step = generator.phase_step;
    }
    assert(sc_tone_generator_frequency_millihz(SC_AUDIO_TONE_COUNT) == 0U);
}

static void generated_samples_are_bounded_and_repeatable(void)
{
    int16_t first[512];
    int16_t second[512];
    sc_tone_generator_t generator;
    sc_tone_generator_init(&generator, SC_AUDIO_TONE_C5);
    sc_tone_generator_fill(&generator, first, 512U);

    bool has_positive = false;
    bool has_negative = false;
    for (size_t i = 0; i < 512U; ++i) {
        assert(first[i] >= -8192 && first[i] <= 8192);
        has_positive = has_positive || first[i] > 0;
        has_negative = has_negative || first[i] < 0;
    }
    assert(has_positive && has_negative);

    sc_tone_generator_set_note(&generator, SC_AUDIO_TONE_C5);
    sc_tone_generator_fill(&generator, second, 512U);
    assert(memcmp(first, second, sizeof(first)) == 0);
}

int main(void)
{
    frequencies_and_octaves_are_correct();
    generated_samples_are_bounded_and_repeatable();
    puts("tone_generator tests passed");
    return 0;
}
