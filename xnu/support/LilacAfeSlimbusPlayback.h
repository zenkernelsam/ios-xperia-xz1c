/* SPDX-License-Identifier: GPL-2.0-only
 * AFE SLIMBUS_0_RX packet/state contract from the pinned Sony q6afe headers.
 * This helper constructs bytes only; its caller owns DSP/codec readiness and IO. */
#ifndef LILAC_AFE_SLIMBUS_PLAYBACK_H
#define LILAC_AFE_SLIMBUS_PLAYBACK_H

#include "LilacAPRProtocol.h"

class LilacAfeSlimbusPlayback {
public:
    enum class State : uint8_t { Unconfigured, Configuring, Configured, Starting, Running, Stopping, Uncertain };
    enum class ParameterSchema : uint8_t { Unknown, V2, V3 };
    enum : uint32_t {
        SetParamV2 = 0x100ef, SetParamV3 = 0x100fa,
        DeviceStart = 0x100e5, DeviceStop = 0x100e6,
        DeviceInterface = 0x1020c, SlimbusConfig = 0x10212,
    };
    enum : uint16_t { Slimbus0Rx = 0x4000 };
    // Sony q6afe sends sizeof(union afe_port_config), which is 36 bytes in
    // the pinned source. The 24-byte SLIMbus member leaves a zero tail.
    enum : unsigned { ConfigurationBytes = 36 };
    struct Configuration {
        uint16_t device;
        uint8_t channels[2];
    };

    LilacAfeSlimbusPlayback() = default;
    LilacAfeSlimbusPlayback(const LilacAfeSlimbusPlayback &) = delete;
    LilacAfeSlimbusPlayback &operator=(const LilacAfeSlimbusPlayback &) = delete;
    State state() const { return phase; }
    bool awaiting_reply() const { return transaction.pending(); }
    bool configured(uint64_t epoch) const
    {
        return epoch && epoch == generation && (phase == State::Configured || phase == State::Running);
    }
    bool running(uint64_t epoch) const { return epoch && epoch == generation && phase == State::Running; }

    size_t configure(uint64_t epoch, uint32_t token, ParameterSchema verified_schema,
        const Configuration &verified_codec_channels, void *packet, size_t capacity)
    {
        if ((phase != State::Unconfigured && phase != State::Configured) ||
            !epoch || (generation && generation != epoch) || token <= last_token ||
            (verified_schema != ParameterSchema::V2 && verified_schema != ParameterSchema::V3) ||
            verified_codec_channels.device > 1 || verified_codec_channels.channels[0] < 128 ||
            verified_codec_channels.channels[1] < 128 ||
            verified_codec_channels.channels[0] == verified_codec_channels.channels[1]) {
            return 0;
        }
        const bool v3 = verified_schema == ParameterSchema::V3;
        uint8_t payload[72]{};
        ADSP::write16(payload, Slimbus0Rx);
        const unsigned header = v3 ? 20 : 16;
        const unsigned parameter = v3 ? 16 : 12;
        // The 12-byte out-of-band mapping header stays zero: this is in-band.
        if (v3) ADSP::write32(payload + 16, parameter + ConfigurationBytes);
        else ADSP::write16(payload + 2, parameter + ConfigurationBytes);
        ADSP::write32(payload + header, DeviceInterface);
        if (v3) {
            // Instance 0 and its reserved word remain zero.
            ADSP::write32(payload + header + 8, SlimbusConfig);
            ADSP::write32(payload + header + 12, ConfigurationBytes);
        } else {
            ADSP::write32(payload + header + 4, SlimbusConfig);
            ADSP::write16(payload + header + 8, ConfigurationBytes);
        }
        uint8_t *config = payload + header + parameter;
        ADSP::write32(config, 1);
        ADSP::write16(config + 4, verified_codec_channels.device);
        ADSP::write16(config + 6, 16);
        ADSP::write16(config + 10, 2);
        config[12] = verified_codec_channels.channels[0];
        config[13] = verified_codec_channels.channels[1];
        ADSP::write32(config + 20, 48000);
        const size_t size = begin(epoch, token, v3 ? SetParamV3 : SetParamV2,
            payload, header + parameter + ConfigurationBytes, State::Configuring, State::Configured, packet, capacity);
        if (size) schema = verified_schema;
        return size;
    }

    size_t start(uint64_t epoch, bool codec_transport_ready, uint32_t token, void *packet, size_t capacity)
    {
        if (phase != State::Configured || epoch != generation || !codec_transport_ready ||
            schema == ParameterSchema::Unknown) return 0;
        uint8_t payload[4]{};
        ADSP::write16(payload, Slimbus0Rx);
        return begin(epoch, token, DeviceStart, payload, sizeof(payload),
            State::Starting, State::Running, packet, capacity);
    }

    size_t stop(uint64_t epoch, bool playback_detached, uint32_t token, void *packet, size_t capacity)
    {
        if (phase != State::Running || epoch != generation || !playback_detached) return 0;
        uint8_t payload[4]{};
        ADSP::write16(payload, Slimbus0Rx);
        return begin(epoch, token, DeviceStop, payload, sizeof(payload),
            State::Stopping, State::Configured, packet, capacity);
    }

    void uncertain()
    {
        if (generation) {
            transaction.fault();
            phase = State::Uncertain;
        }
    }

    ADSP::Result receive(uint64_t epoch, const void *packet, size_t bytes)
    {
        using ADSP::Result;
        if (phase == State::Uncertain) return Result::Quarantined;
        if (!awaiting_reply() || epoch != generation) return Result::Stale;
        ADSP::Packet parsed;
        if (ADSP::parseAPR(packet, bytes, parsed) != Result::OK) return Result::Invalid;
        const auto result = transaction.receive(epoch, parsed);
        if (result != Result::OK) return result;
        // APR Accepted alone never changes the port's lifecycle state.
        if (!transaction.done()) return Result::NotReady;
        if (transaction.result() != Result::OK) {
            const auto failure = transaction.result();
            uncertain();
            return failure;
        }
        phase = next;
        return Result::OK;
    }

private:
    size_t begin(uint64_t epoch, uint32_t token, uint32_t opcode, const void *payload, size_t bytes,
        State pending, State success, void *packet, size_t capacity)
    {
        if (!epoch || token <= last_token || !packet || capacity < ADSP::APRHeader + bytes ||
            transaction.poisoned() || transaction.pending()) return 0;
        if (transaction.begin(epoch, ADSP::AFE, 0, token, opcode, 0) != ADSP::Result::OK) return 0;
        const size_t size = ADSP::command(packet, capacity, ADSP::AFE, 0, token, opcode, payload, bytes);
        if (!size) { transaction.unpublished(); return 0; }
        generation = epoch; last_token = token; phase = pending; next = success;
        return size;
    }
    ADSP::Transaction transaction;
    uint64_t generation = 0;
    uint32_t last_token = 0;
    State phase = State::Unconfigured, next = State::Unconfigured;
    ParameterSchema schema = ParameterSchema::Unknown;
};

#endif
