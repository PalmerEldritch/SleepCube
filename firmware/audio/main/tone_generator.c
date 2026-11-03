#include "tone_generator.h"

#ifndef SC_TONE_SAMPLE_RATE_HZ
#include "board_config.h"
#define SC_TONE_SAMPLE_RATE_HZ SC_AUDIO_SAMPLE_RATE_HZ
#endif

#define SC_TONE_LUT_BITS       6U
#define SC_TONE_LUT_SIZE       (1U << SC_TONE_LUT_BITS)
#define SC_TONE_INDEX_SHIFT    (32U - SC_TONE_LUT_BITS)
#define SC_TONE_FRACTION_SHIFT 10U
#define SC_TONE_PEAK_Q15       8192

/* 64-point sine table with linear interpolation. The generated -12 dBFS
 * samples pass through the normal -12 dBFS product gain cap, producing the
 * selected safe -24 dBFS maximum at 100% volume. */
static const int16_t SINE_LUT[SC_TONE_LUT_SIZE] = {
    0, 3212, 6393, 9512, 12539, 15446, 18204, 20787,
    23170, 25329, 27245, 28898, 30273, 31356, 32137, 32609,
    32767, 32609, 32137, 31356, 30273, 28898, 27245, 25329,
    23170, 20787, 18204, 15446, 12539, 9512, 6393, 3212,
    0, -3212, -6393, -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539, -9512, -6393, -3212,
};

static const uint32_t NOTE_FREQUENCY_MILLIHZ[SC_AUDIO_TONE_COUNT] = {
    130813U, 261626U, 523251U, 1046502U, 2093005U, 4186009U,
};

uint32_t sc_tone_generator_frequency_millihz(sc_audio_tone_note_t note)
{
    return note < SC_AUDIO_TONE_COUNT ? NOTE_FREQUENCY_MILLIHZ[note] : 0U;
}

void sc_tone_generator_set_note(sc_tone_generator_t *generator,
                                sc_audio_tone_note_t note)
{
    if (generator == NULL || note >= SC_AUDIO_TONE_COUNT) {
        return;
    }
    generator->note = note;
    generator->phase = 0U;
    generator->phase_step = (uint32_t)(
        ((uint64_t)NOTE_FREQUENCY_MILLIHZ[note] << 32) /
        ((uint64_t)SC_TONE_SAMPLE_RATE_HZ * 1000U));
}

void sc_tone_generator_init(sc_tone_generator_t *generator,
                            sc_audio_tone_note_t note)
{
    if (generator == NULL) {
        return;
    }
    *generator = (sc_tone_generator_t){0};
    sc_tone_generator_set_note(generator, note);
}

void sc_tone_generator_fill(sc_tone_generator_t *generator,
                            int16_t *samples,
                            size_t frame_count)
{
    if (generator == NULL || samples == NULL) {
        return;
    }
    for (size_t i = 0; i < frame_count; ++i) {
        const uint32_t index = generator->phase >> SC_TONE_INDEX_SHIFT;
        const uint32_t next = (index + 1U) & (SC_TONE_LUT_SIZE - 1U);
        const uint32_t fraction =
            (generator->phase >> SC_TONE_FRACTION_SHIFT) & 0xffffU;
        const int32_t first = SINE_LUT[index];
        const int32_t interpolated = first +
            (int32_t)(((int64_t)(SINE_LUT[next] - first) * fraction) >> 16);
        samples[i] = (int16_t)((interpolated * SC_TONE_PEAK_Q15) >> 15);
        generator->phase += generator->phase_step;
    }
}
