/* SPDX-License-Identifier: GPL-2.0-only
 * Actual AFE/ADM/ASM packet bodies under an authored transport fixture. */
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include "../../xnu/support/LilacPcmPlaybackSession.h"

using Session = LilacPcmPlaybackSession;
using Action = Session::Action;
using State = Session::State;
using Result = ADSP::Result;

static Session::Configuration configuration(LilacAfeSlimbusPlayback::ParameterSchema schema)
{
    return {9, 0x101, 0x00010be4, 0x00010be4, UINT64_C(0x123456000), 8192, schema, {0, {144, 145}}};
}

static size_t response(uint8_t *packet, const uint8_t *request, uint32_t opcode, size_t payload_bytes)
{
    const size_t bytes = ADSP::APRHeader + payload_bytes;
    std::memset(packet, 0, bytes);
    ADSP::write16(packet, 0x150);
    ADSP::write16(packet + 2, bytes);
    packet[4] = packet[8] = request[4];
    packet[5] = 4; packet[9] = 5;
    ADSP::write16(packet + 6, ADSP::read16(request + 10));
    ADSP::write16(packet + 10, ADSP::read16(request + 6));
    ADSP::write32(packet + 12, ADSP::read32(request + 12));
    ADSP::write32(packet + 16, opcode);
    return bytes;
}

static void complete(Session &session, const Session::Request &request, const uint8_t *wire)
{
    uint8_t reply[36]{};
    size_t bytes = response(reply, wire, ADSP::BasicResponse, 8);
    ADSP::write32(reply + 20, request.opcode);
    assert(session.receive(8, reply, bytes) == Result::Stale && session.state() == State::AwaitingReply);
    ADSP::write32(reply + 12, request.token + 1);
    assert(session.receive(9, reply, bytes) == Result::Stale && session.state() == State::AwaitingReply);
    ADSP::write32(reply + 12, request.token);
    if (request.action == Action::MapRegion || request.action == Action::OpenOutput) {
        assert(session.receive(9, reply, bytes) == Result::NotReady && session.state() == State::AwaitingReply);
        if (request.action == Action::MapRegion) {
            bytes = response(reply, wire, ADSP::MapResponse, 4);
            ADSP::write32(reply + 20, 17);
        } else {
            bytes = response(reply, wire, LilacAdmPlaybackDevice::OpenResponseV5, 8);
            ADSP::write16(reply + 24, 3);
        }
    }
    assert(session.receive(9, reply, bytes) == Result::OK);
    assert(session.state() != State::AwaitingReply);
    assert(session.receive(9, reply, bytes) == Result::Stale);
}

static void start(Session &session, bool v3)
{
    const Action actions[] = {Action::OpenStream, Action::SetStreamFormat, Action::MapRegion, Action::ConfigureAfe,
        Action::StartAfe, Action::OpenOutput, Action::RouteOutput, Action::Run};
    const size_t sizes[] = {36, 44, 40, v3 ? 92u : 84u, 24, 48, 36, 32};
    uint8_t storage[120], saved[120];
    for (unsigned i = 0; i < 8; ++i) {
        std::memset(storage, 0xa5, sizeof(storage)); std::memcpy(saved, storage, sizeof(storage));
        Session::Request request;
        assert(session.next_action() == actions[i]);
        assert(!session.build_next(storage + 8, sizes[i] - 1, request));
        assert(session.state() == State::Ready && !std::memcmp(storage, saved, sizeof(storage)));
        assert(session.build_next(storage + 8, sizeof(storage) - 16, request) == sizes[i]);
        assert(request.action == actions[i] && request.bytes == sizes[i]);
        assert(request.service == storage[12] && request.opcode == ADSP::read32(storage + 24));
        assert(session.state() == State::AwaitingReply);
        for (unsigned n = 0; n < 8; ++n) assert(storage[n] == 0xa5);
        for (unsigned n = 8 + sizes[i]; n < sizeof(storage); ++n) assert(storage[n] == 0xa5);
        Session::Request repeat;
        std::memcpy(saved, storage, sizeof(storage));
        assert(!session.build_next(storage + 8, 104, repeat));
        assert(!std::memcmp(storage, saved, sizeof(storage)));
        uint32_t token = 0;
        assert(session.write(0, 4096, 2048, storage + 8, 104, &token) == LILAC_ASM_QUEUE_INVALID);
        assert(!session.begin_drain());
        // Accepted alone must not unlock the next command.
        uint8_t accepted[28];
        size_t bytes = response(accepted, storage + 8, ADSP::Accepted, 0);
        const auto result = session.receive(9, accepted, bytes);
        assert((result == Result::NotReady || result == Result::Stale) && session.state() == State::AwaitingReply);
        complete(session, request, storage + 8);
    }
    assert(session.state() == State::Running && !session.region_release_confirmed());
}

static void write_done(Session &session, uint32_t token, unsigned offset)
{
    uint8_t reply[36]{};
    ADSP::write16(reply, 0x50); ADSP::write16(reply + 2, sizeof(reply));
    reply[4] = reply[8] = ADSP::ASM; reply[5] = 4; reply[9] = 5;
    ADSP::write16(reply + 6, 0x101); ADSP::write16(reply + 10, 0x101);
    ADSP::write32(reply + 12, token); ADSP::write32(reply + 16, ADSP::WriteDone);
    ADSP::write64(reply + 20, UINT64_C(0x123456000) + offset); ADSP::write32(reply + 28, 17);
    assert(session.receive(8, reply, sizeof(reply)) == Result::Stale);
    assert(session.receive(9, reply, sizeof(reply)) == Result::OK);
    assert(session.receive(9, reply, sizeof(reply)) == Result::Stale);
}

static void rendered_eos(Session &session, const Session::Request &request, const uint8_t *wire,
    bool event_first)
{
    uint8_t basic[28]{}, event[24]{};
    const size_t basic_bytes = response(basic, wire, ADSP::BasicResponse, 8);
    ADSP::write32(basic + 20, request.opcode);
    const size_t event_bytes = response(event, wire, LilacAsmPcmStream::RenderedEndOfStream, 0);
    ADSP::write16(event, 0x50);
    assert(request.action == Action::EndOfStream && request.bytes == 20);
    assert(request.opcode == 0x10bdb);
    assert(session.receive(8, event, event_bytes) == Result::Stale);
    ADSP::write32(event + 12, request.token + 1);
    assert(session.receive(9, event, event_bytes) == Result::Stale);
    ADSP::write32(event + 12, request.token);
    ADSP::write16(event, 0x150);
    assert(session.receive(9, event, event_bytes) == Result::Stale);
    ADSP::write16(event, 0x50);
    ADSP::write16(event + 2, event_bytes + 4);
    assert(session.receive(9, event, event_bytes + 4) == Result::Invalid);
    ADSP::write16(event + 2, event_bytes);
    ADSP::write32(event + 16, 0x10bdd); // EOS acceptance is not rendered EOS.
    assert(session.receive(9, event, event_bytes) == Result::Stale);
    ADSP::write32(event + 16, LilacAsmPcmStream::RenderedEndOfStream);
    const void *first = event_first ? static_cast<void *>(event) : static_cast<void *>(basic);
    const void *second = event_first ? static_cast<void *>(basic) : static_cast<void *>(event);
    assert(session.receive(9, first, event_first ? event_bytes : basic_bytes) == Result::NotReady);
    assert(session.state() == State::AwaitingReply && !session.region_release_confirmed());
    uint8_t packet[104];
    Session::Request next;
    uint32_t token;
    assert(!session.build_next(packet, sizeof(packet), next));
    assert(session.write(0, 1024, 256, packet, sizeof(packet), &token) == LILAC_ASM_QUEUE_INVALID);
    assert(session.receive(9, second, event_first ? basic_bytes : event_bytes) == Result::OK);
    assert(session.state() == State::Ready && session.next_action() == Action::CloseStream);
    assert(session.receive(9, event, event_bytes) == Result::Stale);
    assert(session.receive(9, basic, basic_bytes) == Result::Stale);
}

static void command_failure_boundaries()
{
    const Action lifecycle[] = {Action::OpenStream, Action::SetStreamFormat, Action::MapRegion,
        Action::ConfigureAfe, Action::StartAfe, Action::OpenOutput, Action::RouteOutput, Action::Run,
        Action::EndOfStream, Action::CloseStream, Action::UnmapRegion, Action::CloseOutput, Action::StopAfe};
    for (unsigned failure = 0; failure < 13; ++failure) {
        Session session(configuration(LilacAfeSlimbusPlayback::ParameterSchema::V2));
        assert(session.admit(9, true, true));
        uint8_t wire[104]{}, reply[36]{};
        Session::Request request;
        for (unsigned n = 0; n <= failure; ++n) {
            if (n == 8) assert(session.begin_drain() && session.pending_buffers() == 0);
            assert(session.build_next(wire, sizeof(wire), request) && request.action == lifecycle[n]);
            if (n == failure) {
                const size_t bytes = response(reply, wire, ADSP::BasicResponse, 8);
                ADSP::write32(reply + 20, request.opcode);
                ADSP::write32(reply + 24, 1);
                assert(session.receive(9, reply, bytes) == Result::IO);
                assert(session.state() == State::Uncertain && !session.region_release_confirmed());
                ADSP::write32(reply + 24, 0);
                assert(session.receive(9, reply, bytes) == Result::Quarantined);
                assert(!session.build_next(wire, sizeof(wire), request));
            } else if (n == 8) {
                rendered_eos(session, request, wire, false);
            } else {
                complete(session, request, wire);
            }
        }
    }
}

int main()
{
    for (auto schema : {LilacAfeSlimbusPlayback::ParameterSchema::V2, LilacAfeSlimbusPlayback::ParameterSchema::V3}) {
        const auto config = configuration(schema);
        Session session(config);
        uint8_t packet[104]{};
        Session::Request request;
        assert(!session.build_next(packet, sizeof(packet), request));
        assert(!session.admit(8, true, true) && !session.admit(9, false, true) && !session.admit(9, true, false));
        assert(session.admit(9, true, true) && !session.admit(9, true, true));
        start(session, schema == LilacAfeSlimbusPlayback::ParameterSchema::V3);
        uint32_t tokens[4]{};
        for (unsigned n = 0; n < 4; ++n) {
            assert(session.write(n * 1024, 1024, 256, packet, sizeof(packet), &tokens[n]) == LILAC_ASM_QUEUE_RESERVED);
            assert(ADSP::read32(packet + 16) == ADSP::Write && ADSP::read32(packet + 28) == 17);
            assert(ADSP::read32(packet + 32) == 256 && session.pending_buffers() == n + 1);
        }
        uint32_t unused;
        assert(session.write(0, 1024, 256, packet, sizeof(packet), &unused) == LILAC_ASM_QUEUE_BUSY);
        assert(session.begin_drain() && !session.begin_drain());
        assert(session.write(4096, 1024, 256, packet, sizeof(packet), &unused) == LILAC_ASM_QUEUE_INVALID);
        assert(!session.build_next(packet, sizeof(packet), request));
        for (unsigned n = 0; n < 4; ++n) {
            write_done(session, tokens[n], n * 1024);
            assert(session.pending_buffers() == 3 - n);
            if (n < 3) assert(!session.build_next(packet, sizeof(packet), request));
        }
        assert(session.build_next(packet, sizeof(packet), request) == 20);
        rendered_eos(session, request, packet, schema == LilacAfeSlimbusPlayback::ParameterSchema::V3);
        const Action closing[] = {Action::CloseStream, Action::UnmapRegion, Action::CloseOutput, Action::StopAfe};
        const size_t sizes[] = {20, 24, 20, 24};
        for (unsigned n = 0; n < 4; ++n) {
            assert(!session.region_release_confirmed());
            assert(session.build_next(packet, sizeof(packet), request) == sizes[n]);
            assert(request.action == closing[n]);
            complete(session, request, packet);
        }
        assert(session.state() == State::Stopped && session.region_release_confirmed());
        assert(!session.build_next(packet, sizeof(packet), request));
        assert(session.write(0, 1024, 256, packet, sizeof(packet), &unused) == LILAC_ASM_QUEUE_INVALID);
    }
    auto config = configuration(LilacAfeSlimbusPlayback::ParameterSchema::V2);
    Session failed(config);
    assert(failed.admit(9, true, true));
    uint8_t packet[104], reply[28];
    Session::Request request;
    assert(failed.build_next(packet, sizeof(packet), request));
    size_t bytes = response(reply, packet, ADSP::BasicResponse, 8);
    ADSP::write32(reply + 20, request.opcode); ADSP::write32(reply + 24, 1);
    assert(failed.receive(9, reply, bytes) == Result::IO && failed.state() == State::Uncertain);
    ADSP::write32(reply + 24, 0);
    assert(failed.receive(9, reply, bytes) == Result::Quarantined && !failed.region_release_confirmed());
    assert(!failed.build_next(packet, sizeof(packet), request));
    Session lost(config);
    assert(lost.admit(9, true, true));
    start(lost, false);
    uint32_t token;
    assert(lost.write(0, 1024, 256, packet, sizeof(packet), &token) == LILAC_ASM_QUEUE_RESERVED);
    lost.uncertain();
    assert(lost.pending_buffers() == 1 && !lost.region_release_confirmed());
    assert(lost.write(1024, 1024, 256, packet, sizeof(packet), &token) == LILAC_ASM_QUEUE_QUARANTINED);
    assert(lost.receive(9, reply, sizeof(reply)) == Result::Quarantined);
    Session write_failed(configuration(LilacAfeSlimbusPlayback::ParameterSchema::V2));
    assert(write_failed.admit(9, true, true));
    start(write_failed, false);
    assert(write_failed.write(0, 1024, 256, packet, sizeof(packet), &token) == LILAC_ASM_QUEUE_RESERVED);
    bytes = response(reply, packet, ADSP::BasicResponse, 8);
    ADSP::write32(reply + 20, ADSP::Write); ADSP::write32(reply + 24, 1);
    assert(write_failed.receive(9, reply, bytes) == Result::IO);
    assert(write_failed.pending_buffers() == 1 && write_failed.state() == State::Uncertain);
    assert(!write_failed.region_release_confirmed());
    command_failure_boundaries();
    config.stream_port = 0x901; Session wrong_session(config);
    assert(!wrong_session.admit(9, true, true));
    config = configuration(LilacAfeSlimbusPlayback::ParameterSchema::Unknown); Session unknown(config);
    assert(!unknown.admit(9, true, true));
    puts("Lilac PCM session PASS: actual AFE/ADM/ASM packet sequence, exact replies, readiness epoch, four-buffer drain, separate EOS reply and RENDERED_EOS in either order, close/unmap/stop, all 13 command errors and pending WRITE error retain uncertain ownership; no DSP/codec/transport");
}
