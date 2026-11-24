#include <assert.h>

#include "../../xnu/support/LilacAsmPcmQueue.h"

static struct lilac_asm_pcm_event completion(const struct lilac_asm_pcm_buffer *buffer,
    uint32_t token, uint32_t status, uint8_t *bytes)
{
    lilac_asm_store32(bytes, (uint32_t)buffer->dsp_address);
    lilac_asm_store32(bytes + 4, (uint32_t)(buffer->dsp_address >> 32));
    lilac_asm_store32(bytes + 8, buffer->map_handle);
    lilac_asm_store32(bytes + 12, status);
    const struct lilac_asm_pcm_event event = {
        8, LILAC_ASM_WRITE_DONE_V2, token, 0x101, 0x101, 7, 0,
        bytes, LILAC_ASM_WRITE_DONE_BYTES
    };
    return event;
}

int main(void)
{
    struct lilac_asm_pcm_queue queue = {0};
    const struct lilac_asm_pcm_buffer buffers[] = {
        {0x1000, 1, 4096}, {0x2000, 1, 4096}, {0x3000, 2, 4096},
        {0x4000, 2, 4096}, {0x5000, 3, 4096}
    };
    const struct lilac_asm_pcm_buffer overlap = {0x1800, 9, 4096};
    uint8_t payload[LILAC_ASM_WRITE_BYTES], done[LILAC_ASM_WRITE_DONE_BYTES];
    uint32_t tokens[4], token = 0;
    assert(!lilac_asm_pcm_queue_open(&queue, 0, 0x101));
    assert(lilac_asm_pcm_queue_open(&queue, 8, 0x101));
    assert(lilac_asm_pcm_queue_reserve(&queue, buffers, 3, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_INVALID);
    assert(queue.last_token == 0 && token == 0);
    for (unsigned index = 0; index < LILAC_ASM_PCM_MAX_PENDING; ++index) {
        assert(lilac_asm_pcm_queue_reserve(&queue, buffers + index, 2048, payload,
            sizeof(payload), tokens + index) == LILAC_ASM_QUEUE_RESERVED);
        assert(tokens[index] == index + 1);
        assert(lilac_asm_load32(payload + 16) == tokens[index]);
    }
    assert(lilac_asm_pcm_queue_reserve(&queue, buffers + 4, 2048, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_BUSY);

    struct lilac_asm_pcm_event event = completion(buffers + 2, tokens[2], 0, done);
    event.epoch--;
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_STALE);
    event.epoch++;
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_RELEASED);
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_STALE);
    assert(lilac_asm_pcm_queue_reserve(&queue, &overlap, 2048, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_BUSY);
    assert(lilac_asm_pcm_queue_reserve(&queue, buffers + 2, 2048, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_RESERVED);
    assert(token == 5);
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_STALE);
    event = completion(buffers + 2, token, 0, done);
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_RELEASED);

    event = completion(buffers, tokens[0], 1, done);
    done[8] ^= 1;
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_STALE);
    assert(!queue.quarantined);
    done[8] ^= 1;
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_QUARANTINED);
    assert(queue.pending[0].epoch == 8);
    event = completion(buffers + 1, tokens[1], 0, done);
    assert(lilac_asm_pcm_queue_receive(&queue, &event) == LILAC_ASM_QUEUE_QUARANTINED);
    assert(queue.pending[1].epoch == 8 && !lilac_asm_pcm_queue_open(&queue, 9, 0x101));
    assert(lilac_asm_pcm_queue_reserve(&queue, buffers + 4, 2048, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_QUARANTINED);

    struct lilac_asm_pcm_queue timed_out = {0};
    assert(lilac_asm_pcm_queue_open(&timed_out, 8, 0x101));
    assert(lilac_asm_pcm_queue_reserve(&timed_out, buffers, 2048, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_RESERVED);
    lilac_asm_pcm_queue_quarantine(&timed_out);
    event = completion(buffers, token, 0, done);
    assert(lilac_asm_pcm_queue_receive(&timed_out, &event) == LILAC_ASM_QUEUE_QUARANTINED);
    assert(timed_out.pending[0].epoch == 8);

    struct lilac_asm_pcm_queue exhausted = {0};
    assert(lilac_asm_pcm_queue_open(&exhausted, 8, 0x101));
    exhausted.last_token = UINT32_MAX - 1;
    assert(lilac_asm_pcm_queue_reserve(&exhausted, buffers, 2048, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_RESERVED);
    assert(token == UINT32_MAX);
    assert(lilac_asm_pcm_queue_reserve(&exhausted, buffers + 1, 2048, payload,
        sizeof(payload), &token) == LILAC_ASM_QUEUE_QUARANTINED);
    event = completion(buffers, UINT32_MAX, 0, done);
    assert(lilac_asm_pcm_queue_receive(&exhausted, &event) == LILAC_ASM_QUEUE_RELEASED);
}
