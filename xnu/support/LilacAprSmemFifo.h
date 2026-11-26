/* SPDX-License-Identifier: GPL-2.0-only
 * Sony native GLINK FIFO over the retained sweet_adsp_apr_ops callbacks.
 * Fixed 16KiB rings, 8-byte alignment and 16-byte TX reserve follow the
 * locked Sony glink_smem_native_xprt.c and SweetADSPAPR.cpp. The caller
 * owns exclusive uncached SMEM access, barriers, interrupt IO and DSP power.
 * No mapping, reset, cursor initialization or DSP startup occurs here. */
#ifndef LILAC_APR_SMEM_FIFO_H
#define LILAC_APR_SMEM_FIFO_H

#include "ADSPAPRProbe.h"
#include "LilacAPRProtocol.h"
#include "LilacAPRBasebandGLINK.h"

class LilacAprSmemFifo {
public:
    using Result = ADSP::Result;
    enum class State : uint8_t { NotReady, Ready, Uncertain };
    enum class Phase : uint8_t {
        None, Cursors, ReadData, WriteData, PublishTxHead, PublishRxTail, Notify
    };
    struct Progress {
        uint64_t tx_head_publications = 0, rx_tail_publications = 0, notifications = 0;
        Phase failure_phase = Phase::None;
        bool tx_publication_uncertain = false, rx_publication_uncertain = false;
    };

    explicit LilacAprSmemFifo(const sweet_adsp_apr_ops &callbacks) : ops(callbacks) {}
    LilacAprSmemFifo(const LilacAprSmemFifo &) = delete;
    LilacAprSmemFifo &operator=(const LilacAprSmemFifo &) = delete;

    State state() const { return current; }
    bool configuration_valid() const { return valid(); }
    uint64_t generation() const { return ops.generation; }
    const Progress &progress() const { return counts; }

    // Read-only admission. Ownership, mapping and DSP readiness must have
    // been independently confirmed for this exact connection generation.
    Result admit(uint64_t epoch, bool exclusive_endpoint, bool uncached_mapping, bool dsp_ready)
    {
        if (current == State::Uncertain) return Result::Quarantined;
        if (current != State::NotReady || !valid()) return Result::Invalid;
        if (epoch != ops.generation) return Result::Stale;
        if (!exclusive_endpoint || !uncached_mapping || !dsp_ready) return Result::NotReady;
        uint32_t tail = 0, head = 0;
        Result result = cursors(ops.descriptor, ops.tx_bytes, tail, head);
        if (result == Result::OK)
            result = cursors(ops.descriptor + 8, ops.rx_bytes, tail, head);
        if (result != Result::OK) return result;
        current = State::Ready;
        return Result::OK;
    }

    Result available(size_t &bytes)
    {
        bytes = 0;
        if (current != State::Ready) return readiness();
        uint32_t tail = 0, head = 0;
        const Result result = cursors(ops.descriptor + 8, ops.rx_bytes, tail, head);
        if (result == Result::OK) bytes = occupied(tail, head, ops.rx_bytes);
        return result;
    }

    Result peek(void *buffer, size_t bytes)
    {
        if (current != State::Ready) return readiness();
        // Link receives a bounded FIFO prefix, which may contain more than
        // one frame. Copy only after every callback read has succeeded.
        rx_observed = false;
        if (!buffer || !bytes || (bytes & 7u) || bytes > sizeof(rx_snapshot))
            return Result::Invalid;
        uint32_t tail = 0, head = 0;
        Result result = cursors(ops.descriptor + 8, ops.rx_bytes, tail, head);
        if (result != Result::OK) return result;
        if (bytes > occupied(tail, head, ops.rx_bytes)) return Result::Invalid;
        ops.fence(ops.context);
        for (size_t offset = 0; offset < bytes; offset += 4) {
            uint32_t word = 0;
            if (!ops.read32(ops.context, ops.rx + (tail + uint32_t(offset)) % ops.rx_bytes, &word))
                return fault(Phase::ReadData, Result::IO);
            ADSP::write32(rx_snapshot + offset, word);
        }
        auto *out = static_cast<uint8_t *>(buffer);
        for (size_t offset = 0; offset < bytes; ++offset) out[offset] = rx_snapshot[offset];
        Baseband::GLINK::Frame frame;
        if (Baseband::GLINK::parse(rx_snapshot, bytes, frame) == Result::OK) {
            rx_tail = tail;
            rx_frame_bytes = frame.wireBytes;
            rx_observed = true;
        }
        // Link owns protocol rejection. Malformed/partial prefixes cannot
        // subsequently be consumed through this FIFO.
        return Result::OK;
    }

    Result consume(size_t bytes)
    {
        if (current != State::Ready) return readiness();
        if (!rx_observed || !bytes || (bytes & 7u) || bytes != rx_frame_bytes)
            return Result::Invalid;
        uint32_t tail = 0, head = 0;
        Result result = cursors(ops.descriptor + 8, ops.rx_bytes, tail, head);
        if (result != Result::OK) return result;
        if (tail != rx_tail || bytes > occupied(tail, head, ops.rx_bytes))
            return fault(Phase::Cursors, Result::Invalid);
        ops.fence(ops.context);
        if (!ops.write32(ops.context, ops.descriptor + 8, (tail + uint32_t(bytes)) % ops.rx_bytes)) {
            counts.rx_publication_uncertain = true;
            return fault(Phase::PublishRxTail, Result::IO);
        }
        rx_observed = false;
        ++counts.rx_tail_publications;
        result = notify();
        if (result != Result::OK) counts.rx_publication_uncertain = true;
        return result;
    }

    Result send(const void *buffer, size_t bytes)
    {
        if (current != State::Ready) return readiness();
        if (!buffer || bytes < 8 || bytes > sizeof(tx_snapshot)) return Result::Invalid;
        const size_t wire_bytes = (bytes + 7u) & ~size_t(7);
        const auto *source = static_cast<const uint8_t *>(buffer);
        for (size_t offset = 0; offset < wire_bytes; ++offset)
            tx_snapshot[offset] = offset < bytes ? source[offset] : 0;
        Baseband::GLINK::Frame frame;
        if (Baseband::GLINK::parse(tx_snapshot, wire_bytes, frame) != Result::OK ||
            frame.wireBytes != wire_bytes) return Result::Invalid;
        uint32_t tail = 0, head = 0;
        Result result = cursors(ops.descriptor, ops.tx_bytes, tail, head);
        if (result != Result::OK) return result;
        const uint32_t free_bytes = tail <= head ? ops.tx_bytes - head + tail : tail - head;
        // This is the only retryable result: no FIFO or cursor write occurred.
        if (free_bytes < wire_bytes + 16u) return Result::Busy;
        for (size_t offset = 0; offset < wire_bytes; offset += 4) {
            if (!ops.write32(ops.context, ops.tx + (head + uint32_t(offset)) % ops.tx_bytes,
                ADSP::read32(tx_snapshot + offset)))
                return fault(Phase::WriteData, Result::IO);
        }
        ops.fence(ops.context);
        if (!ops.write32(ops.context, ops.descriptor + 4, (head + uint32_t(wire_bytes)) % ops.tx_bytes)) {
            counts.tx_publication_uncertain = true;
            return fault(Phase::PublishTxHead, Result::IO);
        }
        ++counts.tx_head_publications;
        result = notify();
        if (result != Result::OK) counts.tx_publication_uncertain = true;
        return result;
    }

    void uncertain()
    {
        current = State::Uncertain;
        rx_observed = false;
    }

private:
    static uint32_t occupied(uint32_t tail, uint32_t head, uint32_t bytes)
    {
        return head >= tail ? head - tail : bytes - tail + head;
    }
    bool region(uint32_t offset, uint32_t bytes) const
    {
        return bytes && !(offset & 3u) && !(bytes & 3u) && offset >= ops.partition &&
            uint64_t(offset) + bytes <= uint64_t(ops.partition) + ops.partition_bytes;
    }
    bool valid() const
    {
        return ops.generation && ops.partition_bytes &&
            uint64_t(ops.partition) + ops.partition_bytes <= ADSP::SMEMBytes &&
            ops.tx_bytes == 16384 && ops.rx_bytes == 16384 &&
            region(ops.descriptor, 32) && region(ops.tx, ops.tx_bytes) && region(ops.rx, ops.rx_bytes) &&
            !Baseband::overlaps(ops.descriptor, 32, ops.tx, ops.tx_bytes) &&
            !Baseband::overlaps(ops.descriptor, 32, ops.rx, ops.rx_bytes) &&
            !Baseband::overlaps(ops.tx, ops.tx_bytes, ops.rx, ops.rx_bytes) &&
            ops.read32 && ops.write32 && ops.fence && ops.notify;
    }
    Result readiness() const
    {
        return current == State::Uncertain ? Result::Quarantined : Result::NotReady;
    }
    Result fault(Phase phase, Result result)
    {
        counts.failure_phase = phase;
        uncertain();
        return result;
    }
    Result cursors(uint32_t base, uint32_t bytes, uint32_t &tail, uint32_t &head)
    {
        if (!ops.read32(ops.context, base, &tail) || !ops.read32(ops.context, base + 4, &head))
            return fault(Phase::Cursors, Result::IO);
        if (tail >= bytes || head >= bytes || ((tail | head) & 7u))
            return fault(Phase::Cursors, Result::Invalid);
        return Result::OK;
    }
    Result notify()
    {
        ops.fence(ops.context);
        if (!ops.notify(ops.context)) return fault(Phase::Notify, Result::IO);
        ++counts.notifications;
        return Result::OK;
    }
    const sweet_adsp_apr_ops ops;
    Progress counts;
    uint8_t tx_snapshot[ADSP::MaxPacket + 16]{}, rx_snapshot[ADSP::MaxPacket + 16]{};
    uint32_t rx_tail = 0;
    size_t rx_frame_bytes = 0;
    State current = State::NotReady;
    bool rx_observed = false;
};

#endif
