/* SPDX-License-Identifier: GPL-2.0-only
 * Serialized session pump and APR sink for the source-locked ADSP::Link.
 * The caller owns the link, DSP/codec readiness, monotonic clock, physical
 * pages and real FIFO IO. Once admitted, only advance() may poll/send/close
 * that link, so no external poll can accept a reply after its deadline.
 * This helper never starts or resets hardware. */
#ifndef LILAC_PCM_PLAYBACK_TRANSPORT_H
#define LILAC_PCM_PLAYBACK_TRANSPORT_H

#include "LilacPcmPlaybackSession.h"

class LilacPcmPlaybackTransport {
public:
    using Result = ADSP::Result;
    using Session = LilacPcmPlaybackSession;
    enum class State : uint8_t { NotReady, Active, Closing, Stopped, Uncertain };
    enum class Failure : uint8_t {
        None, Clock, Epoch, Readiness, Poll, Publish, PublishTimeout,
        ReplyTimeout, WriteTimeout, CloseTimeout, SessionError
    };
    struct Timeouts {
        uint32_t publish_ms, reply_ms, write_ms, close_ms;
    };

    LilacPcmPlaybackTransport(const Session::Configuration &configuration, Timeouts limits)
        : session(configuration), epoch(configuration.epoch), timeouts(limits)
    {
        valid = limits.publish_ms && limits.reply_ms && limits.write_ms && limits.close_ms;
    }

    LilacPcmPlaybackTransport(const LilacPcmPlaybackTransport &) = delete;
    LilacPcmPlaybackTransport &operator=(const LilacPcmPlaybackTransport &) = delete;
    State state() const { return phase; }
    Session::State session_state() const { return session.state(); }
    Failure failure() const { return failed_at; }
    Result failure_result() const { return failed_result; }
    unsigned pending_buffers() const { return session.pending_buffers(); }
    bool pending_publication() const { return pending.bytes != 0; }
    uint64_t published_packets() const { return publications; }

    template<class Link>
    bool admit(const Link &link, bool dsp_ready, bool codec_ready, uint64_t now_ms)
    {
        if (phase != State::NotReady || !valid || !link.ready() || link.quarantined() ||
            link.generation() != epoch ||
            !session.admit(epoch, dsp_ready, codec_ready)) return false;
        last_ms = now_ms;
        phase = State::Active;
        return true;
    }

    // A prepared APR packet is immutable until send returns OK. Link::Busy
    // guarantees no APR publication; only that result permits retrying the
    // same bytes. IO errors may follow FIFO/head publication and are terminal.
    template<class Link>
    Result advance(Link &link, bool dsp_ready, bool codec_ready, uint64_t now_ms)
    {
        if (phase == State::Uncertain) return Result::Quarantined;
        if (phase == State::Stopped) return Result::OK;
        if (phase == State::NotReady) return Result::NotReady;
        if (!clock(now_ms)) return failed_result;
        if (link.generation() != epoch) return fail(Failure::Epoch, Result::Stale);
        if (!dsp_ready || !codec_ready) return fail(Failure::Readiness, Result::NotReady);
        if (link.quarantined()) return fail(Failure::Poll, Result::Quarantined);
        if (phase == State::Active && !link.ready()) return fail(Failure::Readiness, Result::NotReady);
        if (!deadlines(now_ms)) return failed_result;
        Result result = link.poll();
        if (phase == State::Uncertain) return failed_result;
        if (result != Result::OK) return fail(Failure::Poll, result);
        if (link.generation() != epoch) return fail(Failure::Epoch, Result::Stale);
        if (link.quarantined()) return fail(Failure::Poll, Result::Quarantined);
        if (phase == State::Closing) {
            if (link.closed()) {
                if (!session.region_release_confirmed() || pending.bytes || control_sent ||
                    session.pending_buffers()) return fail(Failure::SessionError, Result::Invalid);
                phase = State::Stopped;
                return Result::OK;
            }
            if (!close_queued) {
                result = link.close();
                if (result == Result::OK) close_queued = true;
                else if (result != Result::Busy) return fail(Failure::Poll, result);
            }
            return Result::NotReady;
        }
        if (!link.ready()) return fail(Failure::Readiness, Result::NotReady);
        if (session.state() == Session::State::Stopped) {
            phase = State::Closing;
            close_started_ms = now_ms;
            result = link.close();
            if (result == Result::OK) close_queued = true;
            else if (result != Result::Busy) return fail(Failure::Poll, result);
            return Result::NotReady;
        }
        if (!pending.bytes && !control_sent &&
            (session.state() == Session::State::Ready || session.state() == Session::State::Draining)) {
            Session::Request request;
            const size_t bytes = session.build_next(packet, sizeof(packet), request);
            if (bytes) {
                pending = {bytes, request.token, false, now_ms};
            } else if (session.state() == Session::State::Ready ||
                (session.state() == Session::State::Draining && !session.pending_buffers())) {
                return fail(Failure::SessionError, Result::Invalid);
            }
        }
        if (!pending.bytes) return Result::OK;
        result = link.send(packet, pending.bytes);
        if (result == Result::Busy) return result;
        if (result != Result::OK) return fail(Failure::Publish, result);
        ++publications;
        if (pending.write) {
            bool installed = false;
            for (auto &write : writes) if (!write.token) {
                write = {pending.token, now_ms};
                installed = true;
                break;
            }
            if (!installed) return fail(Failure::SessionError, Result::Invalid);
        } else {
            control_sent = true;
            control_started_ms = now_ms;
        }
        pending = {};
        return Result::OK;
    }

    lilac_asm_pcm_queue_result queue_write(uint32_t offset, uint32_t span, uint32_t bytes,
        uint64_t now_ms, uint32_t *token)
    {
        if (phase == State::Uncertain) return LILAC_ASM_QUEUE_QUARANTINED;
        if (phase != State::Active || !token) return LILAC_ASM_QUEUE_INVALID;
        if (!clock(now_ms) || !deadlines(now_ms)) return LILAC_ASM_QUEUE_QUARANTINED;
        if (pending.bytes) return LILAC_ASM_QUEUE_BUSY;
        uint32_t reserved = 0;
        const auto result = session.write(offset, span, bytes, packet, sizeof(packet), &reserved);
        if (result == LILAC_ASM_QUEUE_RESERVED) {
            pending = {ADSP::APRHeader + LILAC_ASM_WRITE_BYTES, reserved, true, now_ms};
            *token = reserved;
        } else if (result == LILAC_ASM_QUEUE_QUARANTINED) {
            fail(Failure::SessionError, Result::Quarantined);
        }
        return result;
    }

    bool begin_drain(uint64_t now_ms)
    {
        return phase == State::Active && clock(now_ms) && deadlines(now_ms) &&
            !pending.bytes && !control_sent && session.begin_drain();
    }

    // ADSP::Link calls its sink only with a complete GLINK datagram. A valid
    // incomplete APR transaction is consumed/ACKed as OK at this layer; the
    // session still waits for its exact final response and original deadline.
    Result receive(const void *data, size_t bytes)
    {
        if (phase == State::Uncertain) return Result::Quarantined;
        if (phase != State::Active) return Result::Stale;
        ADSP::Packet reply;
        if (ADSP::parseAPR(data, bytes, reply) != Result::OK)
            return fail(Failure::SessionError, Result::Invalid);
        const bool write_done = reply.service == ADSP::ASM && reply.opcode == ADSP::WriteDone;
        const bool write_error = reply.service == ADSP::ASM &&
            reply.opcode == ADSP::BasicResponse && reply.bytes == 8 &&
            ADSP::read32(reply.payload) == ADSP::Write;
        Write *published_write = nullptr;
        if (write_done || write_error) {
            for (auto &write : writes) if (write.token && write.token == reply.token) {
                published_write = &write;
                break;
            }
            // Reserved-but-unpublished and stale tokens cannot retire pages.
            if (!published_write) return Result::Stale;
        } else if (!control_sent) {
            return Result::Stale;
        }
        const Result result = session.receive(epoch, data, bytes);
        if (session.state() == Session::State::Uncertain ||
            result == Result::IO || result == Result::Quarantined || result == Result::Invalid)
            return fail(Failure::SessionError, result);
        if (write_done && result == Result::OK) published_write->token = 0;
        if (control_sent && session.state() != Session::State::AwaitingReply) control_sent = false;
        return result == Result::NotReady ? Result::OK : result;
    }

    void uncertain() { (void)fail(Failure::Readiness, Result::Quarantined); }
    bool region_release_confirmed() const
    {
        return phase == State::Stopped && session.region_release_confirmed() &&
            !pending.bytes && !control_sent && !session.pending_buffers();
    }

private:
    struct Pending { size_t bytes = 0; uint32_t token = 0; bool write = false; uint64_t since_ms = 0; };
    struct Write { uint32_t token = 0; uint64_t since_ms = 0; };
    Result fail(Failure at, Result result)
    {
        if (phase != State::Uncertain) {
            failed_at = at;
            failed_result = result;
            session.uncertain();
            phase = State::Uncertain;
        }
        return failed_result;
    }
    bool clock(uint64_t now_ms)
    {
        if (now_ms < last_ms) {
            fail(Failure::Clock, Result::Invalid);
            return false;
        }
        last_ms = now_ms;
        return true;
    }
    bool deadlines(uint64_t now_ms)
    {
        if (pending.bytes && now_ms - pending.since_ms >= timeouts.publish_ms) {
            fail(Failure::PublishTimeout, Result::Timeout);
            return false;
        }
        if (control_sent && now_ms - control_started_ms >= timeouts.reply_ms) {
            fail(Failure::ReplyTimeout, Result::Timeout);
            return false;
        }
        for (const auto &write : writes)
            if (write.token && now_ms - write.since_ms >= timeouts.write_ms) {
                fail(Failure::WriteTimeout, Result::Timeout);
                return false;
            }
        if (phase == State::Closing && now_ms - close_started_ms >= timeouts.close_ms) {
            fail(Failure::CloseTimeout, Result::Timeout);
            return false;
        }
        return true;
    }
    Session session;
    const uint64_t epoch;
    const Timeouts timeouts;
    uint8_t packet[104]{};
    Pending pending;
    Write writes[LILAC_ASM_PCM_MAX_PENDING]{};
    uint64_t last_ms = 0, control_started_ms = 0, close_started_ms = 0, publications = 0;
    State phase = State::NotReady;
    Failure failed_at = Failure::None;
    Result failed_result = Result::Quarantined;
    bool valid = false, control_sent = false, close_queued = false;
};

#endif
