#ifndef LILAC_ADM_PLAYBACK_DEVICE_H
#define LILAC_ADM_PLAYBACK_DEVICE_H

#include "LilacAPRProtocol.h"

class LilacAdmPlaybackDevice {
public:
    enum class State : uint8_t { Closed, Opening, Open, Closing, Uncertain };
    enum : uint32_t { OpenV5 = 0x10326, CloseV5 = 0x10327, OpenResponseV5 = 0x10329 };

    LilacAdmPlaybackDevice() = default;
    LilacAdmPlaybackDevice(const LilacAdmPlaybackDevice &) = delete;
    LilacAdmPlaybackDevice &operator=(const LilacAdmPlaybackDevice &) = delete;

    State state() const { return phase; }

    size_t open(uint64_t epoch, uint16_t verified_afe_port, uint32_t verified_topology,
        uint32_t token, void *packet, size_t capacity)
    {
        if (phase != State::Closed || !epoch || !verified_afe_port || verified_afe_port == UINT16_MAX ||
            !verified_topology || (generation && generation != epoch) || token <= last_token) {
            return 0;
        }
        uint8_t payload[28]{};
        ADSP::write16(payload + 2, 1);
        ADSP::write16(payload + 4, verified_afe_port);
        ADSP::write16(payload + 6, UINT16_MAX);
        ADSP::write32(payload + 8, verified_topology);
        ADSP::write16(payload + 12, 2);
        ADSP::write16(payload + 14, 16);
        ADSP::write32(payload + 16, 48000);
        payload[20] = 1;
        payload[21] = 2;
        const size_t size = ADSP::command(packet, capacity, ADSP::ADM, verified_afe_port, token,
            OpenV5, payload, sizeof(payload));
        if (size) {
            generation = epoch;
            afe_port = verified_afe_port;
            last_token = token;
            phase = State::Opening;
        }
        return size;
    }

    size_t close(uint64_t epoch, bool playback_detached, uint32_t token, void *packet, size_t capacity)
    {
        if (!playback_detached || phase != State::Open || epoch != generation || token <= last_token) {
            return 0;
        }
        const size_t size = ADSP::command(packet, capacity, ADSP::ADM, afe_port, token,
            CloseV5, nullptr, 0);
        if (size) {
            ADSP::write16(static_cast<uint8_t *>(packet) + 10, copp);
            last_token = token;
            phase = State::Closing;
        }
        return size;
    }

    bool opened_handle(uint64_t epoch, uint16_t &handle) const
    {
        if (phase != State::Open || epoch != generation) {
            return false;
        }
        handle = copp;
        return true;
    }

    void uncertain()
    {
        if (phase != State::Closed) {
            phase = State::Uncertain;
        }
    }

    ADSP::Result receive(uint64_t epoch, const void *packet, size_t bytes)
    {
        using ADSP::Result;
        if (phase == State::Uncertain) {
            return Result::Quarantined;
        }
        if (epoch != generation || (phase != State::Opening && phase != State::Closing)) {
            return Result::Stale;
        }
        ADSP::Packet parsed;
        if (ADSP::parseAPR(packet, bytes, parsed) != Result::OK) {
            return Result::Invalid;
        }
        const bool opening = phase == State::Opening;
        if (parsed.type != ADSP::Message::Response || parsed.service != ADSP::ADM ||
            parsed.token != last_token || parsed.destinationPort != afe_port ||
            parsed.sourcePort != (opening ? afe_port : copp)) {
            return Result::Stale;
        }
        if (parsed.opcode == ADSP::Accepted) {
            return Result::NotReady;
        }
        if (parsed.opcode == ADSP::BasicResponse) {
            if (parsed.bytes != 8 || ADSP::read32(parsed.payload) != (opening ? OpenV5 : CloseV5)) {
                return Result::Invalid;
            }
            if (ADSP::read32(parsed.payload + 4)) {
                uncertain();
                return Result::IO;
            }
            if (opening) {
                return Result::NotReady;
            }
            phase = State::Closed;
            copp = 0;
            return Result::OK;
        }
        if (!opening || parsed.opcode != OpenResponseV5) {
            return Result::Stale;
        }
        if (parsed.bytes != 8 || ADSP::read16(parsed.payload + 6)) {
            uncertain();
            return Result::Invalid;
        }
        const uint16_t received_copp = ADSP::read16(parsed.payload + 4);
        if (ADSP::read32(parsed.payload) || received_copp == 99 || received_copp >= 255) {
            uncertain();
            return Result::IO;
        }
        copp = received_copp;
        phase = State::Open;
        return Result::OK;
    }

private:
    friend class LilacAdmPlaybackRoute;

    State phase = State::Closed;
    uint64_t generation = 0;
    uint32_t last_token = 0;
    uint16_t afe_port = 0, copp = 0;
};

#endif
