#ifndef LILAC_ASM_PCM_APR_H
#define LILAC_ASM_PCM_APR_H

#include "LilacAPRProtocol.h"
#include "LilacAsmPcmQueue.h"

static_assert(ADSP::Write == LILAC_ASM_WRITE_V2 && ADSP::WriteDone == LILAC_ASM_WRITE_DONE_V2);

inline lilac_asm_pcm_queue_result lilac_asm_pcm_apr_build_write(
    lilac_asm_pcm_queue *queue, const lilac_asm_pcm_buffer *buffer, uint32_t bytes,
    uint8_t *packet, size_t capacity, uint32_t *token)
{
    if (!packet || capacity < ADSP::APRHeader + LILAC_ASM_WRITE_BYTES || !token) {
        return LILAC_ASM_QUEUE_INVALID;
    }
    uint8_t payload[LILAC_ASM_WRITE_BYTES];
    uint32_t reserved_token;
    const auto result = lilac_asm_pcm_queue_reserve(queue, buffer, bytes, payload, sizeof(payload), &reserved_token);
    if (result != LILAC_ASM_QUEUE_RESERVED) {
        return result;
    }
    if (ADSP::command(packet, capacity, ADSP::ASM, queue->port, reserved_token,
            ADSP::Write, payload, sizeof(payload)) != ADSP::APRHeader + sizeof(payload)) {
        lilac_asm_pcm_queue_quarantine(queue);
        return LILAC_ASM_QUEUE_QUARANTINED;
    }
    *token = reserved_token;
    return LILAC_ASM_QUEUE_RESERVED;
}

inline lilac_asm_pcm_queue_result lilac_asm_pcm_apr_receive(
    lilac_asm_pcm_queue *queue, uint64_t connection_epoch, const void *packet, size_t bytes)
{
    if (!queue || !queue->epoch || !connection_epoch) {
        return LILAC_ASM_QUEUE_INVALID;
    }
    if (queue->quarantined) {
        return LILAC_ASM_QUEUE_QUARANTINED;
    }
    if (connection_epoch != queue->epoch) {
        return LILAC_ASM_QUEUE_STALE;
    }
    ADSP::Packet parsed;
    if (ADSP::parseAPR(packet, bytes, parsed) != ADSP::Result::OK) {
        return LILAC_ASM_QUEUE_INVALID;
    }
    if (parsed.opcode != ADSP::WriteDone || parsed.service != ADSP::ASM ||
        parsed.type != ADSP::Message::Event || parsed.bytes != LILAC_ASM_WRITE_DONE_BYTES ||
        bytes != ADSP::APRHeader + LILAC_ASM_WRITE_DONE_BYTES) {
        return LILAC_ASM_QUEUE_STALE;
    }
    const lilac_asm_pcm_event event = {
        connection_epoch, parsed.opcode, parsed.token, parsed.sourcePort, parsed.destinationPort,
        parsed.service, static_cast<uint8_t>(parsed.type), parsed.payload, parsed.bytes
    };
    return lilac_asm_pcm_queue_receive(queue, &event);
}

#endif
