#include "wav_reader.h"

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "app_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "pcm_format.h"
#include "wav_format.h"

static const char *TAG = "wav_reader";

static uint32_t read_le32(const uint8_t *value)
{
    return (uint32_t)value[0] |
           ((uint32_t)value[1] << 8) |
           ((uint32_t)value[2] << 16) |
           ((uint32_t)value[3] << 24);
}

static bool read_exact(FILE *file, void *destination, size_t length)
{
    return fread(destination, 1, length, file) == length;
}

esp_err_t sc_wav_reader_open(sc_wav_reader_t *reader, const char *path)
{
    return sc_wav_reader_open_expected(reader, path,
                                       SC_CONTENT_SAMPLE_RATE_HZ,
                                       SC_CONTENT_BITS_PER_SAMPLE);
}

esp_err_t sc_wav_reader_open_expected(sc_wav_reader_t *reader,
                                      const char *path,
                                      uint32_t expected_sample_rate_hz,
                                      uint16_t expected_bits_per_sample)
{
    ESP_RETURN_ON_FALSE(reader != NULL && path != NULL, ESP_ERR_INVALID_ARG,
                        TAG, "invalid open arguments");
    ESP_RETURN_ON_FALSE(expected_sample_rate_hz > 0U &&
                            (expected_bits_per_sample == 16U ||
                             expected_bits_per_sample == 24U),
                        ESP_ERR_INVALID_ARG, TAG, "invalid expected format");
    ESP_RETURN_ON_FALSE(reader->file == NULL, ESP_ERR_INVALID_STATE, TAG,
                        "reader is already open");

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "cannot open %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t riff_header[12];
    if (!read_exact(file, riff_header, sizeof(riff_header)) ||
        memcmp(riff_header, "RIFF", 4) != 0 ||
        memcmp(&riff_header[8], "WAVE", 4) != 0) {
        ESP_LOGE(TAG, "%s is not a RIFF/WAVE file", path);
        fclose(file);
        return ESP_ERR_INVALID_RESPONSE;
    }

    bool format_found = false;
    bool data_found = false;
    uint16_t audio_format = 0;
    uint16_t valid_bits = 0;
    uint16_t block_align = 0;
    uint32_t byte_rate = 0;
    bool integer_pcm = false;

    while (!data_found) {
        uint8_t chunk_header[8];
        if (!read_exact(file, chunk_header, sizeof(chunk_header))) {
            break;
        }
        const uint32_t chunk_size = read_le32(&chunk_header[4]);
        const long chunk_data_offset = ftell(file);
        if (chunk_data_offset < 0) {
            break;
        }

        if (memcmp(chunk_header, "fmt ", 4) == 0) {
            uint8_t format_data[40];
            const size_t format_bytes =
                chunk_size < sizeof(format_data) ? chunk_size
                                                 : sizeof(format_data);
            sc_wav_format_t format;
            if (chunk_size < 16U ||
                !read_exact(file, format_data, format_bytes) ||
                !sc_wav_format_parse(format_data, format_bytes, &format)) {
                break;
            }
            audio_format = format.format_tag;
            integer_pcm = format.integer_pcm;
            reader->channel_count = format.channel_count;
            reader->sample_rate_hz = format.sample_rate_hz;
            byte_rate = format.byte_rate;
            block_align = format.block_align;
            reader->bits_per_sample = format.container_bits;
            valid_bits = format.valid_bits;
            format_found = true;
        } else if (memcmp(chunk_header, "data", 4) == 0) {
            if (!format_found || chunk_data_offset > UINT32_MAX || chunk_size == 0) {
                break;
            }
            reader->data_offset = (uint32_t)chunk_data_offset;
            reader->data_size = chunk_size;
            data_found = true;
            break;
        }

        const uint64_t next_chunk = (uint64_t)(unsigned long)chunk_data_offset +
                                    chunk_size + (chunk_size & 1U);
        if (next_chunk > LONG_MAX || fseek(file, (long)next_chunk, SEEK_SET) != 0) {
            break;
        }
    }

    const uint16_t expected_block_align =
        SC_CONTENT_CHANNEL_COUNT * (expected_bits_per_sample / 8U);
    const uint32_t expected_byte_rate =
        expected_sample_rate_hz * expected_block_align;
    if (!format_found || !data_found || !integer_pcm ||
        reader->channel_count != SC_CONTENT_CHANNEL_COUNT ||
        reader->sample_rate_hz != expected_sample_rate_hz ||
        reader->bits_per_sample != expected_bits_per_sample ||
        valid_bits != expected_bits_per_sample ||
        block_align != expected_block_align || byte_rate != expected_byte_rate ||
        (reader->data_size % expected_block_align) != 0U) {
        ESP_LOGE(TAG,
                 "unsupported WAV: tag=0x%04x pcm=%u channels=%u rate=%" PRIu32
                 "Hz container=%u valid=%u align=%u byte_rate=%" PRIu32
                 "; require PCM mono %u-bit %" PRIu32 "Hz",
                 audio_format, integer_pcm ? 1U : 0U,
                 (unsigned)reader->channel_count, reader->sample_rate_hz,
                 (unsigned)reader->bits_per_sample, (unsigned)valid_bits,
                 (unsigned)block_align, byte_rate,
                 (unsigned)expected_bits_per_sample,
                 expected_sample_rate_hz);
        fclose(file);
        memset(reader, 0, sizeof(*reader));
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        ESP_LOGE(TAG, "cannot determine content file size");
        fclose(file);
        memset(reader, 0, sizeof(*reader));
        return ESP_FAIL;
    }
    const long file_size = ftell(file);
    const uint64_t declared_end = (uint64_t)reader->data_offset +
                                  (uint64_t)reader->data_size;
    if (file_size < 0 || declared_end > (uint64_t)(unsigned long)file_size) {
        ESP_LOGE(TAG,
                 "truncated WAV data: declared_end=%" PRIu64 " file_size=%ld",
                 declared_end, file_size);
        fclose(file);
        memset(reader, 0, sizeof(*reader));
        return ESP_ERR_INVALID_SIZE;
    }

    reader->file = file;
    reader->bytes_remaining = reader->data_size;
    if (fseek(reader->file, (long)reader->data_offset, SEEK_SET) != 0) {
        sc_wav_reader_close(reader);
        return ESP_FAIL;
    }

    const uint32_t frame_count = reader->data_size / expected_block_align;
    ESP_LOGI(TAG,
             "content ready: path=%s PCM mono signed-%u rate=%" PRIu32
             "Hz frames=%" PRIu32 " duration_ms=%" PRIu32,
             path,
             (unsigned)reader->bits_per_sample,
             reader->sample_rate_hz,
             frame_count,
             (uint32_t)(((uint64_t)frame_count * 1000U) / reader->sample_rate_hz));
    return ESP_OK;
}

esp_err_t sc_wav_reader_read_q31(sc_wav_reader_t *reader,
                                 int32_t *frames,
                                 size_t capacity_frames,
                                 size_t *frames_read)
{
    ESP_RETURN_ON_FALSE(reader != NULL && reader->file != NULL &&
                            frames != NULL && frames_read != NULL,
                        ESP_ERR_INVALID_ARG, TAG, "invalid read arguments");
    *frames_read = 0;
    if (capacity_frames == 0 || reader->bytes_remaining == 0) {
        return ESP_OK;
    }

    const size_t bytes_per_frame = reader->bits_per_sample / 8U;
    const size_t available_frames = reader->bytes_remaining / bytes_per_frame;
    const size_t requested_frames = capacity_frames < available_frames
                                        ? capacity_frames : available_frames;
    uint8_t raw[SC_PLAYBACK_BLOCK_FRAMES * 3U];
    size_t converted = 0U;
    while (converted < requested_frames) {
        size_t chunk_frames = requested_frames - converted;
        if (chunk_frames > SC_PLAYBACK_BLOCK_FRAMES) {
            chunk_frames = SC_PLAYBACK_BLOCK_FRAMES;
        }
        const size_t requested_bytes = chunk_frames * bytes_per_frame;
        const size_t bytes_read = fread(raw, 1, requested_bytes, reader->file);
        if (bytes_read != requested_bytes) {
            ESP_LOGE(TAG, "content read failed: requested=%u read=%u",
                     (unsigned)requested_bytes, (unsigned)bytes_read);
            return ESP_FAIL;
        }
        for (size_t i = 0; i < chunk_frames; ++i) {
            const uint8_t *sample = &raw[i * bytes_per_frame];
            frames[converted + i] = sc_pcm_le_sample_to_q31(
                sample, (uint8_t)reader->bits_per_sample);
        }
        reader->bytes_remaining -= requested_bytes;
        converted += chunk_frames;
    }
    *frames_read = converted;
    return ESP_OK;
}

esp_err_t sc_wav_reader_rewind(sc_wav_reader_t *reader)
{
    ESP_RETURN_ON_FALSE(reader != NULL && reader->file != NULL,
                        ESP_ERR_INVALID_STATE, TAG, "reader is not open");
    clearerr(reader->file);
    if (fseek(reader->file, (long)reader->data_offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "content rewind failed");
        return ESP_FAIL;
    }
    reader->bytes_remaining = reader->data_size;
    return ESP_OK;
}

void sc_wav_reader_close(sc_wav_reader_t *reader)
{
    if (reader == NULL) {
        return;
    }
    if (reader->file != NULL) {
        fclose(reader->file);
    }
    memset(reader, 0, sizeof(*reader));
}
