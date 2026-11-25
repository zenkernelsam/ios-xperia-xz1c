/* SPDX-License-Identifier: GPL-2.0-only
 * Sony ASM PCM protocol lifecycle; no DSP power or transport operations. */
#ifndef LILAC_ASM_PCM_STREAM_H
#define LILAC_ASM_PCM_STREAM_H

#include "LilacAsmPcmMap.h"
#include "LilacAdmPlaybackRoute.h"

class LilacAsmPcmStream {
public:
    enum class State : uint8_t { Closed, Open, Configured, Running, Uncertain };
    enum : uint32_t { EndOfStream = 0x10bdb, RenderedEndOfStream = 0x10c1c };

    LilacAsmPcmStream(uint64_t epoch, uint16_t port)
    {
        if ((port >> 8) >= 1 && (port >> 8) <= 15 && (port & 255u) >= 1 && (port & 255u) <= 8) {
            lilac_asm_pcm_queue_open(&queue, epoch, port);
        }
    }

    LilacAsmPcmStream(const LilacAsmPcmStream &) = delete;
    LilacAsmPcmStream &operator=(const LilacAsmPcmStream &) = delete;

    State state() const { return queue.quarantined ? State::Uncertain : current; }
    bool awaiting_reply() const { return command_opcode != 0 || (eos_requested && !eos_rendered); }
    unsigned pending_buffers() const
    {
        unsigned count = 0;
        for (const auto &entry : queue.pending) count += entry.epoch != 0;
        return count;
    }
    bool render_drain_confirmed() const
    {
        return state() == State::Running && eos_requested && eos_rendered && !awaiting_reply();
    }

    size_t open(uint32_t verified_topology, void *packet, size_t capacity)
    {
        if (state() != State::Closed || mapping.state() != LilacAsmPcmMap::State::Empty || !verified_topology) {
            return 0;
        }
        uint8_t payload[16]{};
        ADSP::write16(payload + 6, 16);
        ADSP::write32(payload + 8, verified_topology);
        ADSP::write32(payload + 12, ADSP::PCMFormat);
        const size_t size = begin(ADSP::OpenWrite, State::Open, payload, sizeof(payload), packet, capacity);
        if (size) {
            eos_requested = eos_rendered = false;
            eos_token = 0;
        }
        return size;
    }

    size_t configure(void *packet, size_t capacity)
    {
        if (state() != State::Open) {
            return 0;
        }
        uint8_t payload[24]{};
        ADSP::write32(payload, 20);
        ADSP::write16(payload + 4, 2);
        ADSP::write16(payload + 6, 16);
        ADSP::write32(payload + 8, 48000);
        ADSP::write16(payload + 12, 1);
        payload[16] = 1;
        payload[17] = 2;
        return begin(ADSP::FormatUpdate, State::Configured, payload, sizeof(payload), packet, capacity);
    }

    size_t map(uint64_t address, uint32_t bytes, void *packet, size_t capacity)
    {
        return state() == State::Configured && !awaiting_reply()
            ? mapping.map(queue, address, bytes, packet, capacity) : 0;
    }

    size_t run(LilacAdmPlaybackRoute &route, void *packet, size_t capacity)
    {
        if (!route.available(queue.epoch, queue.port >> 8) || state() != State::Configured ||
            mapping.state() != LilacAsmPcmMap::State::Mapped) {
            return 0;
        }
        const uint8_t payload[12]{};
        const size_t size = begin(ADSP::Run, State::Running, payload, sizeof(payload), packet, capacity);
        if (size) {
            route.claim();
            running_route = &route;
        }
        return size;
    }

    lilac_asm_pcm_queue_result write(const LilacAdmPlaybackRoute &route, uint32_t offset, uint32_t span, uint32_t bytes,
        uint8_t *packet, size_t capacity, uint32_t *token)
    {
        if (state() == State::Uncertain) {
            return LILAC_ASM_QUEUE_QUARANTINED;
        }
        if (state() != State::Running || eos_requested || awaiting_reply() || running_route != &route ||
            !route.confirmed(queue.epoch, queue.port >> 8)) {
            return LILAC_ASM_QUEUE_INVALID;
        }
        return mapping.write(offset, span, bytes, packet, capacity, token);
    }

    size_t end_of_stream(void *packet, size_t capacity)
    {
        if (state() != State::Running || eos_requested || pending_buffers()) return 0;
        const size_t size = begin(EndOfStream, State::Running, nullptr, 0, packet, capacity);
        if (size) {
            eos_requested = true;
            eos_token = queue.last_token;
        }
        return size;
    }

    size_t close(void *packet, size_t capacity)
    {
        if (state() == State::Closed || state() == State::Uncertain ||
            (eos_requested && !render_drain_confirmed()) ||
            mapping.state() == LilacAsmPcmMap::State::Mapping || mapping.state() == LilacAsmPcmMap::State::Unmapping) {
            return 0;
        }
        for (const auto &entry : queue.pending) {
            if (entry.epoch) {
                return 0;
            }
        }
        return begin(ADSP::Close, State::Closed, nullptr, 0, packet, capacity);
    }

    size_t unmap(void *packet, size_t capacity)
    {
        return state() == State::Closed && !awaiting_reply() ? mapping.unmap(packet, capacity) : 0;
    }

    bool region_release_confirmed() const
    {
        return state() == State::Closed && !awaiting_reply() && mapping.state() == LilacAsmPcmMap::State::Empty;
    }

    void uncertain()
    {
        lilac_asm_pcm_queue_quarantine(&queue);
        mapping.uncertain();
    }

    ADSP::Result receive(uint64_t epoch, const void *packet, size_t bytes)
    {
        using ADSP::Result;
        if (state() == State::Uncertain) {
            return Result::Quarantined;
        }
        if (!queue.epoch || epoch != queue.epoch) {
            return Result::Stale;
        }
        ADSP::Packet received;
        if (ADSP::parseAPR(packet, bytes, received) != Result::OK) {
            return Result::Invalid;
        }
        if (received.service != ADSP::ASM) {
            return Result::Stale;
        }
        if (!received.sourcePort && !received.destinationPort) {
            return mapping.receive(epoch, packet, bytes);
        }
        if (received.sourcePort != queue.port || received.destinationPort != queue.port) {
            return Result::Stale;
        }
        // The pinned Sony PCM driver waits for RENDERED_EOS separately from
        // the basic EOS command response. Either may arrive first; both must
        // belong to this request before the stream can be closed as drained.
        if (received.opcode == RenderedEndOfStream) {
            if (!eos_requested || eos_rendered || received.token != eos_token ||
                received.type != ADSP::Message::Event || state() != State::Running)
                return Result::Stale;
            if (received.bytes) return Result::Invalid;
            eos_rendered = true;
            return command_opcode ? Result::NotReady : Result::OK;
        }
        if (received.opcode == ADSP::WriteDone && state() == State::Running && !awaiting_reply()) {
            const auto result = lilac_asm_pcm_apr_receive(&queue, epoch, packet, bytes);
            return result == LILAC_ASM_QUEUE_RELEASED ? Result::OK :
                result == LILAC_ASM_QUEUE_QUARANTINED ? Result::Quarantined :
                result == LILAC_ASM_QUEUE_INVALID ? Result::Invalid : Result::Stale;
        }
        if (received.type != ADSP::Message::Response || received.opcode != ADSP::BasicResponse) {
            return Result::Stale;
        }
        if (received.bytes != 8) {
            return Result::Invalid;
        }
        const uint32_t opcode = ADSP::read32(received.payload);
        const uint32_t status = ADSP::read32(received.payload + 4);
        if (opcode == ADSP::Write && status) {
            for (const auto &entry : queue.pending) {
                if (entry.epoch && entry.token == received.token) {
                    uncertain();
                    return Result::IO;
                }
            }
        }
        if (!awaiting_reply() || received.token != queue.last_token || opcode != command_opcode) {
            return Result::Stale;
        }
        if (status) {
            uncertain();
            return Result::IO;
        }
        current = next_state;
        if (current == State::Closed) {
            running_route = nullptr;
        }
        command_opcode = 0;
        return awaiting_reply() ? Result::NotReady : Result::OK;
    }

private:
    size_t begin(uint32_t opcode, State next, const void *payload, size_t bytes, void *packet, size_t capacity)
    {
        if (!queue.epoch || queue.quarantined || awaiting_reply() || queue.last_token == UINT32_MAX) {
            return 0;
        }
        const size_t size = ADSP::command(packet, capacity, ADSP::ASM, queue.port, queue.last_token + 1,
            opcode, payload, bytes);
        if (size) {
            ++queue.last_token;
            command_opcode = opcode;
            next_state = next;
        }
        return size;
    }

    lilac_asm_pcm_queue queue{};
    LilacAsmPcmMap mapping;
    const LilacAdmPlaybackRoute *running_route = nullptr;
    State current = State::Closed, next_state = State::Closed;
    uint32_t command_opcode = 0;
    uint32_t eos_token = 0;
    bool eos_requested = false, eos_rendered = false;
};

#endif
