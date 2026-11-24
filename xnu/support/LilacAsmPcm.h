#ifndef LILAC_ASM_PCM_H
#define LILAC_ASM_PCM_H

#include <stddef.h>
#include <stdint.h>

enum {
    LILAC_ASM_WRITE_V2 = 0x10dab,
    LILAC_ASM_WRITE_DONE_V2 = 0x10d99,
    LILAC_ASM_WRITE_BYTES = 32,
    LILAC_ASM_WRITE_DONE_BYTES = 16
};

struct lilac_asm_pcm_buffer {
    uint64_t dsp_address;
    uint32_t map_handle;
    uint32_t size;
};

struct lilac_asm_pcm_submission {
    struct lilac_asm_pcm_buffer buffer;
    uint64_t epoch;
    uint32_t token;
    uint16_t port;
};

struct lilac_asm_pcm_event {
    uint64_t epoch;
    uint32_t opcode, token;
    uint16_t source_port, destination_port;
    uint8_t service, message_type;
    const uint8_t *payload;
    size_t bytes;
};

enum lilac_asm_completion {
    LILAC_ASM_UNMATCHED,
    LILAC_ASM_CONSUMED,
    LILAC_ASM_DSP_ERROR
};

static inline void lilac_asm_store32(uint8_t *bytes, uint32_t value)
{
    for (unsigned index = 0; index < 4; ++index) {
        bytes[index] = (uint8_t)(value >> (8 * index));
    }
}

static inline uint32_t lilac_asm_load32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static inline int lilac_asm_pcm_buffer_valid(const struct lilac_asm_pcm_buffer *buffer)
{
    const uint64_t limit = UINT64_C(1) << 36;
    return buffer && buffer->dsp_address && buffer->dsp_address < limit &&
        !(buffer->dsp_address & 31u) && buffer->map_handle &&
        buffer->size && !(buffer->size & 3u) && buffer->size <= limit - buffer->dsp_address;
}

static inline size_t lilac_asm_pcm_write_payload(uint8_t *out, size_t capacity,
    const struct lilac_asm_pcm_buffer *buffer, uint32_t bytes, uint32_t sequence)
{
    if (!out || capacity < LILAC_ASM_WRITE_BYTES || !lilac_asm_pcm_buffer_valid(buffer) ||
        !bytes || (bytes & 3u) || bytes > buffer->size) {
        return 0;
    }
    lilac_asm_store32(out, (uint32_t)buffer->dsp_address);
    lilac_asm_store32(out + 4, (uint32_t)(buffer->dsp_address >> 32));
    lilac_asm_store32(out + 8, buffer->map_handle);
    lilac_asm_store32(out + 12, bytes);
    lilac_asm_store32(out + 16, sequence);
    lilac_asm_store32(out + 20, 0);
    lilac_asm_store32(out + 24, 0);
    lilac_asm_store32(out + 28, 0);
    return LILAC_ASM_WRITE_BYTES;
}

static inline enum lilac_asm_completion lilac_asm_pcm_write_done(
    const uint8_t *payload, size_t bytes, const struct lilac_asm_pcm_buffer *buffer)
{
    if (!payload || bytes != LILAC_ASM_WRITE_DONE_BYTES || !lilac_asm_pcm_buffer_valid(buffer) ||
        lilac_asm_load32(payload) != (uint32_t)buffer->dsp_address ||
        lilac_asm_load32(payload + 4) != (uint32_t)(buffer->dsp_address >> 32) ||
        lilac_asm_load32(payload + 8) != buffer->map_handle) {
        return LILAC_ASM_UNMATCHED;
    }
    return lilac_asm_load32(payload + 12) ? LILAC_ASM_DSP_ERROR : LILAC_ASM_CONSUMED;
}

static inline enum lilac_asm_completion lilac_asm_pcm_complete(
    const struct lilac_asm_pcm_submission *submission, const struct lilac_asm_pcm_event *event)
{
    if (!submission || !event || !submission->epoch || !submission->port ||
        event->epoch != submission->epoch || event->token != submission->token ||
        event->source_port != submission->port || event->destination_port != submission->port ||
        event->opcode != LILAC_ASM_WRITE_DONE_V2 || event->service != 7 || event->message_type != 0) {
        return LILAC_ASM_UNMATCHED;
    }
    return lilac_asm_pcm_write_done(event->payload, event->bytes, &submission->buffer);
}

#endif
