#ifndef LILAC_ASM_PCM_QUEUE_H
#define LILAC_ASM_PCM_QUEUE_H

#include "LilacAsmPcm.h"

enum { LILAC_ASM_PCM_MAX_PENDING = 4 };

struct lilac_asm_pcm_queue {
    uint64_t epoch;
    uint32_t last_token;
    uint16_t port;
    uint8_t quarantined;
    struct lilac_asm_pcm_submission pending[LILAC_ASM_PCM_MAX_PENDING];
};

enum lilac_asm_pcm_queue_result {
    LILAC_ASM_QUEUE_INVALID,
    LILAC_ASM_QUEUE_BUSY,
    LILAC_ASM_QUEUE_RESERVED,
    LILAC_ASM_QUEUE_STALE,
    LILAC_ASM_QUEUE_RELEASED,
    LILAC_ASM_QUEUE_QUARANTINED
};

static inline int lilac_asm_pcm_queue_open(struct lilac_asm_pcm_queue *queue, uint64_t epoch, uint16_t port)
{
    if (!queue || queue->epoch || !epoch || !port) {
        return 0;
    }
    queue->port = port;
    queue->epoch = epoch;
    return 1;
}

static inline void lilac_asm_pcm_queue_quarantine(struct lilac_asm_pcm_queue *queue)
{
    if (queue && queue->epoch) {
        queue->quarantined = 1;
    }
}

static inline enum lilac_asm_pcm_queue_result lilac_asm_pcm_queue_reserve(
    struct lilac_asm_pcm_queue *queue, const struct lilac_asm_pcm_buffer *buffer,
    uint32_t bytes, uint8_t *payload, size_t capacity, uint32_t *token)
{
    if (!queue || !queue->epoch || !token || !lilac_asm_pcm_buffer_valid(buffer)) {
        return LILAC_ASM_QUEUE_INVALID;
    }
    if (queue->quarantined || queue->last_token == UINT32_MAX) {
        return LILAC_ASM_QUEUE_QUARANTINED;
    }
    struct lilac_asm_pcm_submission *slot = NULL;
    const struct lilac_asm_pcm_buffer mapped = *buffer;
    for (unsigned index = 0; index < LILAC_ASM_PCM_MAX_PENDING; ++index) {
        struct lilac_asm_pcm_submission *item = &queue->pending[index];
        if (!item->epoch) {
            if (!slot) {
                slot = item;
            }
        } else if (mapped.dsp_address < item->buffer.dsp_address + item->buffer.size &&
                   item->buffer.dsp_address < mapped.dsp_address + mapped.size) {
            return LILAC_ASM_QUEUE_BUSY;
        }
    }
    if (!slot) {
        return LILAC_ASM_QUEUE_BUSY;
    }
    const uint32_t next_token = queue->last_token + 1;
    if (!lilac_asm_pcm_write_payload(payload, capacity, &mapped, bytes, next_token)) {
        return LILAC_ASM_QUEUE_INVALID;
    }
    slot->buffer = mapped;
    slot->epoch = queue->epoch;
    slot->port = queue->port;
    slot->token = next_token;
    queue->last_token = next_token;
    *token = next_token;
    return LILAC_ASM_QUEUE_RESERVED;
}

static inline enum lilac_asm_pcm_queue_result lilac_asm_pcm_queue_receive(
    struct lilac_asm_pcm_queue *queue, const struct lilac_asm_pcm_event *event)
{
    if (!queue || !queue->epoch || !event) {
        return LILAC_ASM_QUEUE_INVALID;
    }
    if (queue->quarantined) {
        return LILAC_ASM_QUEUE_QUARANTINED;
    }
    if (event->epoch != queue->epoch) {
        return LILAC_ASM_QUEUE_STALE;
    }
    for (unsigned index = 0; index < LILAC_ASM_PCM_MAX_PENDING; ++index) {
        struct lilac_asm_pcm_submission *item = &queue->pending[index];
        if (!item->epoch || item->token != event->token) {
            continue;
        }
        const enum lilac_asm_completion result = lilac_asm_pcm_complete(item, event);
        if (result == LILAC_ASM_CONSUMED) {
            item->epoch = 0;
            return LILAC_ASM_QUEUE_RELEASED;
        }
        if (result == LILAC_ASM_DSP_ERROR) {
            lilac_asm_pcm_queue_quarantine(queue);
            return LILAC_ASM_QUEUE_QUARANTINED;
        }
        break;
    }
    return LILAC_ASM_QUEUE_STALE;
}

#endif
