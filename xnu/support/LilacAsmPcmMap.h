#ifndef LILAC_ASM_PCM_MAP_H
#define LILAC_ASM_PCM_MAP_H

#include "LilacAsmPcmApr.h"

class LilacAsmPcmMap {
public:
    enum class State : uint8_t { Empty, Mapping, Mapped, Unmapping, Uncertain };

    LilacAsmPcmMap() = default;
    LilacAsmPcmMap(const LilacAsmPcmMap &) = delete;
    LilacAsmPcmMap &operator=(const LilacAsmPcmMap &) = delete;

    State state() const { return phase; }

    size_t map(lilac_asm_pcm_queue &queue, uint64_t address, uint32_t bytes, void *packet, size_t capacity)
    {
        const uint64_t limit = UINT64_C(1) << 36;
        if (phase != State::Empty || !queue.epoch || (owner && owner != &queue) ||
            (epoch && epoch != queue.epoch) || !idle(queue) ||
            !address || address >= limit || !bytes || ((address | bytes) & 4095u) ||
            bytes > limit - address) {
            return 0;
        }
        uint8_t payload[20]{};
        ADSP::write16(payload, 3);
        ADSP::write16(payload + 2, 1);
        ADSP::write64(payload + 8, address);
        ADSP::write32(payload + 16, bytes);
        const size_t size = begin(packet, capacity, ADSP::Map, payload, sizeof(payload), State::Mapping);
        if (size) {
            owner = &queue;
            epoch = queue.epoch;
            base = address;
            length = bytes;
        }
        return size;
    }

    size_t unmap(void *packet, size_t capacity)
    {
        if (phase != State::Mapped || owner->epoch != epoch || !idle(*owner)) {
            return 0;
        }
        uint8_t payload[4];
        ADSP::write32(payload, handle);
        return begin(packet, capacity, ADSP::Unmap, payload, sizeof(payload), State::Unmapping);
    }

    lilac_asm_pcm_queue_result write(uint32_t offset, uint32_t span, uint32_t bytes,
        uint8_t *packet, size_t capacity, uint32_t *write_token)
    {
        if (phase == State::Uncertain || (owner && owner->quarantined)) {
            return LILAC_ASM_QUEUE_QUARANTINED;
        }
        if (phase != State::Mapped || owner->epoch != epoch || offset > length || span > length - offset) {
            return LILAC_ASM_QUEUE_INVALID;
        }
        const lilac_asm_pcm_buffer buffer{base + offset, handle, span};
        return lilac_asm_pcm_apr_build_write(owner, &buffer, bytes, packet, capacity, write_token);
    }

    void uncertain()
    {
        if (phase != State::Empty) {
            phase = State::Uncertain;
            lilac_asm_pcm_queue_quarantine(owner);
        }
    }

    ADSP::Result receive(uint64_t connection_epoch, const void *packet, size_t bytes)
    {
        using ADSP::Result;
        if (phase == State::Uncertain) {
            return Result::Quarantined;
        }
        if ((phase != State::Mapping && phase != State::Unmapping) ||
            connection_epoch != epoch || owner->epoch != epoch) {
            return Result::Stale;
        }
        if (owner->quarantined) {
            uncertain();
            return Result::Quarantined;
        }
        ADSP::Packet reply;
        if (ADSP::parseAPR(packet, bytes, reply) != Result::OK) {
            return Result::Invalid;
        }
        if (reply.service != ADSP::ASM || reply.sourcePort || reply.destinationPort ||
            reply.token != token || reply.type != ADSP::Message::Response) {
            return Result::Stale;
        }
        if (reply.opcode == ADSP::Accepted) {
            return Result::NotReady;
        }
        if (reply.opcode == ADSP::BasicResponse) {
            if (reply.bytes != 8 || ADSP::read32(reply.payload) != (phase == State::Mapping ? ADSP::Map : ADSP::Unmap)) {
                return Result::Invalid;
            }
            if (ADSP::read32(reply.payload + 4)) {
                uncertain();
                return Result::IO;
            }
            if (phase == State::Mapping) {
                return Result::NotReady;
            }
            phase = State::Empty;
            base = length = handle = 0;
            return Result::OK;
        }
        if (reply.opcode != ADSP::MapResponse || phase != State::Mapping) {
            return Result::Stale;
        }
        if (reply.bytes != 4 || !(handle = ADSP::read32(reply.payload))) {
            uncertain();
            return Result::Invalid;
        }
        phase = State::Mapped;
        return Result::OK;
    }

private:
    static bool idle(const lilac_asm_pcm_queue &queue)
    {
        if (queue.quarantined) {
            return false;
        }
        for (const auto &entry : queue.pending) {
            if (entry.epoch) {
                return false;
            }
        }
        return true;
    }

    size_t begin(void *packet, size_t capacity, uint32_t opcode, const void *payload, size_t bytes, State next)
    {
        if (token == UINT32_MAX) {
            return 0;
        }
        const size_t size = ADSP::command(packet, capacity, ADSP::ASM, 0, token + 1, opcode, payload, bytes);
        if (size) {
            ++token;
            phase = next;
        }
        return size;
    }

    lilac_asm_pcm_queue *owner = nullptr;
    State phase = State::Empty;
    uint64_t epoch = 0, base = 0;
    uint32_t length = 0, handle = 0, token = 0;
};

#endif
