/* SPDX-License-Identifier: GPL-2.0-only
 * One stereo S16/48kHz playback lifecycle over the pinned Sony AFE/ADM/ASM
 * APR contracts. The session owns protocol state, not DSP power, codec IO,
 * physical pages or the transport. No packet is sent by this header. */
#ifndef LILAC_PCM_PLAYBACK_SESSION_H
#define LILAC_PCM_PLAYBACK_SESSION_H

#include "LilacAfeSlimbusPlayback.h"
#include "LilacAsmPcmStream.h"

class LilacPcmPlaybackSession {
public:
    enum class State : uint8_t { NotReady, Ready, AwaitingReply, Running, Draining, Stopped, Uncertain };
    enum class Action : uint8_t {
        None, OpenStream, SetStreamFormat, MapRegion, ConfigureAfe, StartAfe,
        OpenOutput, RouteOutput, Run, EndOfStream, CloseStream, UnmapRegion, CloseOutput, StopAfe
    };
    struct Configuration {
        uint64_t epoch;
        uint16_t stream_port;
        uint32_t stream_topology, output_topology;
        uint64_t region_address;
        uint32_t region_bytes;
        LilacAfeSlimbusPlayback::ParameterSchema parameter_schema;
        LilacAfeSlimbusPlayback::Configuration codec_channels;
    };
    struct Request {
        Action action = Action::None;
        size_t bytes = 0;
        uint8_t service = 0;
        uint32_t token = 0, opcode = 0;
    };

    explicit LilacPcmPlaybackSession(const Configuration &configuration)
        : config(configuration), stream(configuration.epoch, configuration.stream_port)
    {
        const uint64_t limit = UINT64_C(1) << 36;
        const unsigned session = config.stream_port >> 8, id = config.stream_port & 255;
        valid = config.epoch && session >= 1 && session <= 8 && id >= 1 && id <= 8 &&
            config.stream_topology && config.output_topology && config.region_address &&
            config.region_address < limit && config.region_bytes &&
            !((config.region_address | config.region_bytes) & 4095) &&
            config.region_bytes <= limit - config.region_address &&
            (config.parameter_schema == LilacAfeSlimbusPlayback::ParameterSchema::V2 ||
             config.parameter_schema == LilacAfeSlimbusPlayback::ParameterSchema::V3) &&
            config.codec_channels.device <= 1 && config.codec_channels.channels[0] >= 128 &&
            config.codec_channels.channels[1] >= 128 &&
            config.codec_channels.channels[0] != config.codec_channels.channels[1];
    }

    LilacPcmPlaybackSession(const LilacPcmPlaybackSession &) = delete;
    LilacPcmPlaybackSession &operator=(const LilacPcmPlaybackSession &) = delete;
    State state() const { return phase; }
    Action next_action() const { return next; }
    unsigned pending_buffers() const { return stream.pending_buffers(); }

    // The transport owner supplies readiness from this exact connection
    // epoch after its separate DSP/codec validation, before any APR command.
    bool admit(uint64_t readiness_epoch, bool dsp_ready, bool codec_ready)
    {
        if (phase != State::NotReady || !valid || readiness_epoch != config.epoch ||
            !dsp_ready || !codec_ready) return false;
        phase = State::Ready;
        next = Action::OpenStream;
        return true;
    }

    size_t build_next(void *packet, size_t capacity, Request &request)
    {
        request = {};
        if (phase == State::Draining && stream.pending_buffers()) return 0;
        if (phase != State::Ready && phase != State::Draining) return 0;
        const Action action = next;
        size_t bytes = 0;
        const bool output_command = action == Action::ConfigureAfe || action == Action::StartAfe ||
            action == Action::OpenOutput || action == Action::RouteOutput ||
            action == Action::CloseOutput || action == Action::StopAfe;
        if (output_command && output_token == UINT32_MAX) return 0;
        const uint32_t token = output_token + 1;
        switch (action) {
        case Action::OpenStream:
            bytes = stream.open(config.stream_topology, packet, capacity); break;
        case Action::SetStreamFormat:
            bytes = stream.configure(packet, capacity); break;
        case Action::MapRegion:
            bytes = stream.map(config.region_address, config.region_bytes, packet, capacity); break;
        case Action::ConfigureAfe:
            bytes = afe.configure(config.epoch, token, config.parameter_schema, config.codec_channels, packet, capacity); break;
        case Action::StartAfe:
            bytes = afe.start(config.epoch, true, token, packet, capacity); break;
        case Action::OpenOutput:
            if (afe.running(config.epoch))
                bytes = output.open(config.epoch, LilacAfeSlimbusPlayback::Slimbus0Rx,
                    config.output_topology, token, packet, capacity);
            break;
        case Action::RouteOutput:
            if (afe.running(config.epoch))
                bytes = route.begin(config.epoch, config.stream_port >> 8, output, token, packet, capacity);
            break;
        case Action::Run:
            if (afe.running(config.epoch)) bytes = stream.run(route, packet, capacity);
            break;
        case Action::EndOfStream:
            bytes = stream.end_of_stream(packet, capacity); break;
        case Action::CloseStream:
            if (stream.render_drain_confirmed())
                bytes = stream.close(packet, capacity);
            break;
        case Action::UnmapRegion:
            bytes = stream.unmap(packet, capacity); break;
        case Action::CloseOutput:
            if (stream.region_release_confirmed())
                bytes = output.close(config.epoch, true, token, packet, capacity);
            break;
        case Action::StopAfe:
            if (stream.region_release_confirmed() && output.state() == LilacAdmPlaybackDevice::State::Closed)
                bytes = afe.stop(config.epoch, true, token, packet, capacity);
            break;
        default: break;
        }
        if (!bytes) return 0;
        if (output_command) ++output_token;
        const auto *raw = static_cast<const uint8_t *>(packet);
        request = {action, bytes, raw[4], ADSP::read32(raw + 12), ADSP::read32(raw + 16)};
        pending_action = action;
        phase = State::AwaitingReply;
        return bytes;
    }

    ADSP::Result receive(uint64_t epoch, const void *packet, size_t bytes)
    {
        using ADSP::Result;
        if (phase == State::Uncertain) return Result::Quarantined;
        if (epoch != config.epoch) return Result::Stale;
        if (phase == State::Running || phase == State::Draining) {
            const auto result = stream.receive(epoch, packet, bytes);
            if (stream.state() == LilacAsmPcmStream::State::Uncertain) uncertain();
            return result;
        }
        if (phase != State::AwaitingReply) return Result::Stale;
        Result result = Result::Stale;
        bool complete = false;
        switch (pending_action) {
        case Action::ConfigureAfe:
        case Action::StartAfe:
        case Action::StopAfe:
            result = afe.receive(epoch, packet, bytes);
            complete = result == Result::OK && !afe.awaiting_reply();
            break;
        case Action::OpenOutput:
        case Action::CloseOutput:
            result = output.receive(epoch, packet, bytes);
            complete = result == Result::OK;
            break;
        case Action::RouteOutput:
            result = route.receive(epoch, packet, bytes);
            complete = result == Result::OK && route.confirmed(epoch, config.stream_port >> 8);
            break;
        default:
            result = stream.receive(epoch, packet, bytes);
            complete = result == Result::OK && !stream.awaiting_reply();
            break;
        }
        if (afe.state() == LilacAfeSlimbusPlayback::State::Uncertain ||
            output.state() == LilacAdmPlaybackDevice::State::Uncertain ||
            stream.state() == LilacAsmPcmStream::State::Uncertain ||
            result == Result::Quarantined || result == Result::IO) {
            uncertain();
            return result;
        }
        if (!complete) return result == Result::OK ? Result::NotReady : result;
        if (pending_action == Action::Run) {
            phase = State::Running;
            next = Action::None;
        } else if (pending_action == Action::StopAfe) {
            phase = State::Stopped;
            next = Action::None;
        } else {
            phase = State::Ready;
            next = following(pending_action);
        }
        pending_action = Action::None;
        return Result::OK;
    }

    lilac_asm_pcm_queue_result write(uint32_t offset, uint32_t span, uint32_t bytes,
        uint8_t *packet, size_t capacity, uint32_t *token)
    {
        if (phase == State::Uncertain) return LILAC_ASM_QUEUE_QUARANTINED;
        if (phase != State::Running || !afe.running(config.epoch)) return LILAC_ASM_QUEUE_INVALID;
        const auto result = stream.write(route, offset, span, bytes, packet, capacity, token);
        if (result == LILAC_ASM_QUEUE_QUARANTINED) uncertain();
        return result;
    }

    bool begin_drain()
    {
        if (phase != State::Running) return false;
        phase = State::Draining;
        next = Action::EndOfStream;
        return true;
    }

    // Lost DSP/link/codec readiness, publication errors or timeouts are never
    // repaired by a late reply. The owner must retain its physical region.
    void uncertain()
    {
        afe.uncertain();
        output.uncertain();
        route.uncertain();
        stream.uncertain();
        phase = State::Uncertain;
    }

    bool region_release_confirmed() const
    {
        return phase == State::Stopped && stream.region_release_confirmed() &&
            output.state() == LilacAdmPlaybackDevice::State::Closed &&
            afe.state() == LilacAfeSlimbusPlayback::State::Configured;
    }

private:
    static Action following(Action action)
    {
        switch (action) {
        case Action::OpenStream: return Action::SetStreamFormat;
        case Action::SetStreamFormat: return Action::MapRegion;
        case Action::MapRegion: return Action::ConfigureAfe;
        case Action::ConfigureAfe: return Action::StartAfe;
        case Action::StartAfe: return Action::OpenOutput;
        case Action::OpenOutput: return Action::RouteOutput;
        case Action::RouteOutput: return Action::Run;
        case Action::EndOfStream: return Action::CloseStream;
        case Action::CloseStream: return Action::UnmapRegion;
        case Action::UnmapRegion: return Action::CloseOutput;
        case Action::CloseOutput: return Action::StopAfe;
        default: return Action::None;
        }
    }
    const Configuration config;
    LilacAfeSlimbusPlayback afe;
    LilacAdmPlaybackDevice output;
    LilacAdmPlaybackRoute route;
    LilacAsmPcmStream stream;
    uint32_t output_token = 0;
    State phase = State::NotReady;
    Action next = Action::None, pending_action = Action::None;
    bool valid = false;
};

#endif
