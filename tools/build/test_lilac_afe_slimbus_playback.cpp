/* SPDX-License-Identifier: GPL-2.0-only; authored byte/lifecycle regressions. */
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include "../../xnu/support/LilacAfeSlimbusPlayback.h"

static void reply(uint8_t *packet, uint32_t token, uint32_t opcode, uint32_t status = 0)
{
    std::memset(packet, 0, 28);
    ADSP::write16(packet, 0x150); ADSP::write16(packet + 2, 28);
    packet[4] = packet[8] = ADSP::AFE; packet[5] = 4; packet[9] = 5;
    ADSP::write32(packet + 12, token); ADSP::write32(packet + 16, ADSP::BasicResponse);
    ADSP::write32(packet + 20, opcode); ADSP::write32(packet + 24, status);
}

int main()
{
    using Port = LilacAfeSlimbusPlayback;
    using Result = ADSP::Result;
    constexpr uint64_t epoch = 7;
    // These are authored channel inputs, not evidence of a selected codec.
    const Port::Configuration config = {0, {144, 145}};
    for (auto schema : {Port::ParameterSchema::V2, Port::ParameterSchema::V3}) {
        Port port;
        uint8_t storage[128], answer[28];
        std::memset(storage, 0xa5, sizeof(storage));
        uint8_t *packet = storage + 8;
        assert(!port.start(epoch, true, 1, packet, 100));
        assert(!port.configure(epoch, 1, Port::ParameterSchema::Unknown, config, packet, 100));
        assert(!port.configure(epoch, 1, schema, {2, {144, 145}}, packet, 100));
        assert(!port.configure(epoch, 1, schema, {0, {127, 145}}, packet, 100));
        assert(!port.configure(epoch, 1, schema, {0, {144, 144}}, packet, 100));
        assert(!port.configure(epoch, 1, schema, config, packet, 71));
        for (uint8_t value : storage) assert(value == 0xa5);
        bool v3 = schema == Port::ParameterSchema::V3;
        const unsigned size = v3 ? 92 : 84;
        const unsigned header = v3 ? 40 : 36;
        const unsigned configuration = v3 ? 56 : 48;
        const auto opcode = v3 ? Port::SetParamV3 : Port::SetParamV2;
        assert(port.configure(epoch, 1, schema, config, packet, 100) == size);
        uint8_t expected[92]{};
        ADSP::write16(expected, 0x250); ADSP::write16(expected + 2, size);
        expected[4] = expected[8] = ADSP::AFE; expected[5] = 5; expected[9] = 4;
        ADSP::write32(expected + 12, 1); ADSP::write32(expected + 16, opcode);
        ADSP::write16(expected + 20, 0x4000);
        if (v3) ADSP::write32(expected + 36, 52);
        else ADSP::write16(expected + 22, 48);
        ADSP::write32(expected + header, 0x1020c);
        ADSP::write32(expected + header + (v3 ? 8 : 4), 0x10212);
        if (v3) ADSP::write32(expected + header + 12, 36);
        else ADSP::write16(expected + header + 8, 36);
        const uint8_t literal_config[24] = {
            1,0,0,0, 0,0,16,0, 0,0,2,0, 144,145,0,0,0,0,0,0, 128,187,0,0,
        };
        std::memcpy(expected + configuration, literal_config, sizeof(literal_config));
        assert(!std::memcmp(packet, expected, size));
        for (unsigned i = 0; i < 8; ++i) assert(storage[i] == 0xa5);
        for (unsigned i = 8 + size; i < sizeof(storage); ++i) assert(storage[i] == 0xa5);
        assert(port.state() == Port::State::Configuring && !port.configured(epoch));
        assert(!port.start(epoch, true, 2, packet, 100));
        reply(answer, 1, opcode);
        assert(port.receive(epoch - 1, answer, sizeof(answer)) == Result::Stale);
        ADSP::write16(answer + 6, 0x4000);
        assert(port.receive(epoch, answer, sizeof(answer)) == Result::Stale);
        ADSP::write16(answer + 6, 0);
        ADSP::write32(answer + 16, ADSP::Accepted);
        assert(port.receive(epoch, answer, sizeof(answer)) == Result::NotReady && port.awaiting_reply());
        reply(answer, 1, opcode);
        assert(port.receive(epoch, answer, sizeof(answer)) == Result::OK && port.configured(epoch));
        assert(!port.configured(epoch - 1) && port.receive(epoch, answer, sizeof(answer)) == Result::Stale);
        assert(!port.start(epoch, false, 2, packet, 100) && !port.start(epoch, true, 1, packet, 100));
        assert(port.start(epoch, true, 2, packet, 100) == 24);
        assert(ADSP::read16(packet + 6) == 0 && ADSP::read16(packet + 10) == 0 &&
               ADSP::read16(packet + 20) == 0x4000 && ADSP::read16(packet + 22) == 0);
        reply(answer, 2, Port::DeviceStart);
        assert(port.receive(epoch, answer, sizeof(answer)) == Result::OK && port.running(epoch));
        assert(!port.configure(epoch, 3, schema, config, packet, 100));
        assert(!port.stop(epoch, false, 3, packet, 100));
        assert(port.stop(epoch, true, 3, packet, 100) == 24 && !port.running(epoch));
        reply(answer, 3, Port::DeviceStop);
        assert(port.receive(epoch, answer, sizeof(answer)) == Result::OK && port.configured(epoch));
        assert(!port.configure(epoch + 1, 4, schema, config, packet, 100));
        assert(port.start(epoch, true, 4, packet, 100) == 24);
        reply(answer, 4, Port::DeviceStart, 1);
        assert(port.receive(epoch, answer, sizeof(answer)) == Result::IO && port.state() == Port::State::Uncertain);
        reply(answer, 4, Port::DeviceStart);
        assert(port.receive(epoch, answer, sizeof(answer)) == Result::Quarantined);
        assert(!port.start(epoch, true, 5, packet, 100) && !port.stop(epoch, true, 5, packet, 100));
    }
    Port timed_out; uint8_t packet[92], answer[28];
    assert(timed_out.configure(epoch, 1, Port::ParameterSchema::V2, config, packet, sizeof(packet)) == 84);
    timed_out.uncertain(); reply(answer, 1, Port::SetParamV2);
    assert(timed_out.receive(epoch, answer, sizeof(answer)) == Result::Quarantined);
    puts("Lilac AFE SLIMBUS PASS: V2/V3 exact packets/guards, original zero APR ports, codec readiness, accepted not completion, token/epoch/error/uncertain states; no transport or DSP");
}
