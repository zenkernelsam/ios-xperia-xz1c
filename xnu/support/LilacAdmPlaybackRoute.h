#ifndef LILAC_ADM_PLAYBACK_ROUTE_H
#define LILAC_ADM_PLAYBACK_ROUTE_H

#include "LilacAdmPlaybackDevice.h"

class LilacAdmPlaybackRoute {
public:
    LilacAdmPlaybackRoute() = default;
    LilacAdmPlaybackRoute(const LilacAdmPlaybackRoute &) = delete;
    LilacAdmPlaybackRoute &operator=(const LilacAdmPlaybackRoute &) = delete;

    size_t begin(uint64_t epoch, uint16_t session, const LilacAdmPlaybackDevice &device, uint32_t token,
        void *packet, size_t capacity)
    {
        uint16_t handle;
        if (!device.opened_handle(epoch, handle)) {
            return 0;
        }
        const size_t size = begin_for_handle(epoch, session, handle, token, packet, capacity);
        if (size) {
            output = &device;
            open_token = device.last_token;
        }
        return size;
    }

private:
    size_t begin_for_handle(uint64_t epoch, uint16_t session, uint16_t opened_copp, uint32_t token,
        void *packet, size_t capacity)
    {
        if (phase != Phase::Empty || session < 1 || session > 8 || opened_copp == 99 || opened_copp >= 255) {
            return 0;
        }
        uint8_t payload[16]{};
        ADSP::write32(payload + 4, 1);
        ADSP::write16(payload + 8, session);
        ADSP::write16(payload + 10, 1);
        ADSP::write16(payload + 12, opened_copp);
        if (transaction.begin(epoch, ADSP::ADM, 0, token, ADSP::MatrixMap, 0) != ADSP::Result::OK) {
            return 0;
        }
        const size_t size = ADSP::command(packet, capacity, ADSP::ADM, 0, token,
            ADSP::MatrixMap, payload, sizeof(payload));
        if (!size) {
            transaction.unpublished();
            return 0;
        }
        generation = epoch;
        session_id = session;
        phase = Phase::Pending;
        return size;
    }

public:
    ADSP::Result receive(uint64_t epoch, const void *packet, size_t bytes)
    {
        using ADSP::Result;
        if (phase == Phase::Uncertain) {
            return Result::Quarantined;
        }
        if (phase != Phase::Pending || epoch != generation) {
            return Result::Stale;
        }
        if (!output_current()) {
            uncertain();
            return Result::Quarantined;
        }
        ADSP::Packet parsed;
        if (ADSP::parseAPR(packet, bytes, parsed) != Result::OK) {
            return Result::Invalid;
        }
        const auto result = transaction.receive(epoch, parsed);
        if (result != Result::OK) {
            return result;
        }
        if (!transaction.done()) {
            return Result::NotReady;
        }
        if (transaction.result() != Result::OK) {
            uncertain();
            return Result::IO;
        }
        phase = Phase::Confirmed;
        return Result::OK;
    }

    bool confirmed(uint64_t epoch, uint16_t session) const
    {
        return phase == Phase::Confirmed && generation == epoch && session_id == session && output_current();
    }

    bool available(uint64_t epoch, uint16_t session) const
    {
        return !claimed && confirmed(epoch, session);
    }

    void uncertain()
    {
        phase = Phase::Uncertain;
        transaction.fault();
    }

private:
    friend class LilacAsmPcmStream;

    bool output_current() const
    {
        return output && output->state() == LilacAdmPlaybackDevice::State::Open &&
            output->generation == generation && output->last_token == open_token;
    }

    void claim()
    {
        claimed = true;
    }

    enum class Phase : uint8_t { Empty, Pending, Confirmed, Uncertain };
    Phase phase = Phase::Empty;
    ADSP::Transaction transaction;
    const LilacAdmPlaybackDevice *output = nullptr;
    uint32_t open_token = 0;
    uint64_t generation = 0;
    uint16_t session_id = 0;
    bool claimed = false;
};

#endif
