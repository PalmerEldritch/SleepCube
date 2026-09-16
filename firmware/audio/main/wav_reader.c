#include "wav_reader.h"

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "app_config.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "wav_reader";

static uint16_t read_le16(const uint8_t *value)
{
    return (uint16_t)value[0] | ((uint16_t)value[1] << 8);
}

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
    ESP_RETURN_ON_FALSE(reader != NULL && path != NULL, ESP_ERR_INVALID_ARG,
                        TAG, "invalid open arguments");
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
    uint16_t block_align = 0;
    uint32_t byte_rate = 0;

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
            uint8_t format[16];
            if (chunk_size < sizeof(format) ||
                !read_exact(file, format, sizeof(format))) {
                break;
            }
            audio_format = read_le16(&format[0]);
            reader->channel_count = read_le16(&format[2]);
            reader->sample_rate_hz = read_le32(&format[4]);
            byte_rate = read_le32(&format[8]);
            block_align = read_le16(&format[12]);
            reader->bits_per_sample = read_le16(&format[14]);
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
        SC_CONTENT_CHANNEL_COUNT * (SC_CONTENT_BITS_PER_SAMPLE / 8U);
    const uint32_t expected_byte_rate =
        SC_CONTENT_SAMPLE_RATE_HZ * expected_block_align;
    if (!format_found || !data_found || audio_format != 1U ||
        reader->channel_count != SC_CONTENT_CHANNEL_COUNT ||
        reader->sample_rate_hz != SC_CONTENT_SAMPLE_RATE_HZ ||
        reader->bits_per_sample != SC_CONTENT_BITS_PER_SAMPLE ||
        block_align != expected_block_align || byte_rate != expected_byte_rate ||
        (reader->data_size % expected_block_align) != 0U) {
        ESP_LOGE(TAG,
                 "unsupported WAV: require PCM mono 16-bit 44100 Hz with aligned data");
        fclose(file);
        memset(reader, 0, sizeof(*reader));
        return ESP_ERR_NOT_SUPPORTED;
    }

    reader->file = file;
    reader->bytes_remaining = reader->data_size;
    if (fseek(reader->file, (long)reader->data_offset, SEEK_SET) != 0) {
        sc_wav_reader_close(reader);
        return ESP_FAIL;
    }

    const uint32_t frame_count = reader->data_size / expected_block_align;
    ESP_LOGI(TAG,
             "content ready: path=%s PCM mono signed-16 rate=%" PRIu32
             "Hz frames=%" PRIu32 " duration_ms=%" PRIu32,
             path,
             reader->sample_rate_hz,
             frame_count,
             (uint32_t)(((uint64_t)frame_count * 1000U) / reader->sample_rate_hz));
    return ESP_OK;
}

esp_err_t sc_wav_reader_read(sc_wav_reader_t *reader,
                             int16_t *frames,
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

    const size_t bytes_per_frame = SC_CONTENT_BITS_PER_SAMPLE / 8U;
    size_t requested_bytes = capacity_frames * bytes_per_frame;
    if (requested_bytes > reader->bytes_remaining) {
        requested_bytes = reader->bytes_remaining;
    }

    const size_t bytes_read = fread(frames, 1, requested_bytes, reader->file);
    if (bytes_read != requested_bytes) {
        ESP_LOGE(TAG, "content read failed: requested=%u read=%u",
                 (unsigned)requested_bytes, (unsigned)bytes_read);
        return ESP_FAIL;
    }

    reader->bytes_remaining -= bytes_read;
    *frames_read = bytes_read / bytes_per_frame;
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
