#include <cassert>
#include <cstring>

#include "../../xnu/support/LilacAsmPcmStream.h"

static void response(uint8_t *packet, size_t bytes, uint16_t port, uint32_t token, uint32_t opcode)
{
    std::memset(packet, 0, bytes);
    ADSP::write16(packet, 0x150);
    ADSP::write16(packet + 2, bytes);
    packet[4] = packet[8] = ADSP::ASM;
    packet[5] = 4;
    packet[9] = 5;
    ADSP::write16(packet + 6, port);
    ADSP::write16(packet + 10, port);
    ADSP::write32(packet + 12, token);
    ADSP::write32(packet + 16, opcode);
}

int main()
{
    using ADSP::Result;
    using State = LilacAsmPcmStream::State;
    constexpr uint64_t epoch = 9, address = 0x123456000;
    constexpr uint16_t port = 0x101;
    constexpr uint32_t topology = 0x00010be4;
    LilacAsmPcmStream stream(epoch, port);
    LilacAdmPlaybackRoute route;
    LilacAdmPlaybackDevice device;
    uint8_t packet[52]{}, reply[36]{};
    uint32_t token = 0;
    auto ack = [&](uint32_t opcode, uint32_t status = 0) {
        response(reply, 28, port, ADSP::read32(packet + 12), ADSP::BasicResponse);
        ADSP::write32(reply + 20, opcode);
        ADSP::write32(reply + 24, status);
    };
    assert(!stream.configure(packet, sizeof(packet)));
    assert(!stream.open(0, packet, sizeof(packet)));
    assert(stream.open(topology, packet, sizeof(packet)) == 36);
    const uint8_t open[] = {0, 0, 0, 0, 0, 0, 16, 0, 0xe4, 0x0b, 1, 0, 0xa5, 0x0d, 1, 0};
    assert(!std::memcmp(packet + 20, open, sizeof(open)));
    assert(stream.state() == State::Closed && stream.awaiting_reply() && !stream.region_release_confirmed());
    response(reply, 24, port, 1, ADSP::Accepted);
    assert(stream.receive(epoch, reply, 24) == Result::Stale);
    assert(!stream.configure(packet, sizeof(packet)));
    ack(ADSP::OpenWrite);
    assert(stream.receive(epoch - 1, reply, 28) == Result::Stale);
    assert(stream.receive(epoch, reply, 28) == Result::OK);
    assert(stream.receive(epoch, reply, 28) == Result::Stale);
    assert(stream.configure(packet, sizeof(packet)) == 44);
    const uint8_t format[] = {20, 0, 0, 0, 2, 0, 16, 0, 0x80, 0xbb, 0, 0, 1, 0, 0, 0, 1, 2};
    assert(!std::memcmp(packet + 20, format, sizeof(format)));
    for (size_t index = 20 + sizeof(format); index < 44; ++index) {
        assert(packet[index] == 0);
    }
    assert(!stream.map(address, 8192, packet, sizeof(packet)));
    ack(ADSP::FormatUpdate);
    assert(stream.receive(epoch, reply, 28) == Result::OK);
    assert(stream.map(address, 8192, packet, sizeof(packet)) == 40);
    assert(!stream.close(packet, sizeof(packet)));
    assert(!stream.run(route, packet, sizeof(packet)));
    response(reply, 24, 0, ADSP::read32(packet + 12), ADSP::MapResponse);
    ADSP::write32(reply + 20, 17);
    assert(stream.receive(epoch, reply, 24) == Result::OK);
    assert(!stream.run(route, packet, sizeof(packet)));
    constexpr uint16_t afe_port = 0x4000;
    assert(!route.begin(epoch, 1, device, 0x123, packet, sizeof(packet)));
    assert(!device.open(epoch, afe_port, topology, 1, packet, 47));
    assert(device.open(epoch, afe_port, topology, 1, packet, sizeof(packet)) == 48);
    const uint8_t device_open[] = {
        0x50, 2, 48, 0, 8, 5, 0, 0x40, 8, 4, 0, 0x40, 1, 0, 0, 0, 0x26, 3, 1, 0,
        0, 0, 1, 0, 0, 0x40, 0xff, 0xff, 0xe4, 0x0b, 1, 0, 2, 0, 16, 0,
        0x80, 0xbb, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0
    };
    assert(!std::memcmp(packet, device_open, sizeof(device_open)));
    response(reply, 28, afe_port, 1, ADSP::BasicResponse);
    reply[4] = reply[8] = ADSP::ADM;
    ADSP::write32(reply + 20, LilacAdmPlaybackDevice::OpenV5);
    assert(device.receive(epoch, reply, 28) == Result::NotReady);
    assert(!route.begin(epoch, 1, device, 0x123, packet, sizeof(packet)));
    response(reply, 28, afe_port, 1, LilacAdmPlaybackDevice::OpenResponseV5);
    reply[4] = reply[8] = ADSP::ADM;
    ADSP::write16(reply + 24, 3);
    assert(device.receive(epoch - 1, reply, 28) == Result::Stale);
    assert(device.receive(epoch, reply, 27) == Result::Invalid);
    assert(device.receive(epoch, reply, 28) == Result::OK);
    assert(device.receive(epoch, reply, 28) == Result::Stale);
    assert(!route.begin(epoch - 1, 1, device, 0x123, packet, sizeof(packet)));
    assert(route.begin(epoch, 1, device, 0x123, packet, sizeof(packet)) == 36);
    const uint8_t matrix[] = {0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 3, 0, 0, 0};
    assert(!std::memcmp(packet + 20, matrix, sizeof(matrix)));
    assert(packet[4] == ADSP::ADM && ADSP::read32(packet + 16) == ADSP::MatrixMap);
    response(reply, 24, 0, 0x123, ADSP::Accepted);
    reply[4] = reply[8] = ADSP::ADM;
    assert(route.receive(epoch, reply, 24) == Result::NotReady);
    assert(!stream.run(route, packet, sizeof(packet)));
    response(reply, 28, 0, 0x123, ADSP::BasicResponse);
    reply[4] = reply[8] = ADSP::ADM;
    ADSP::write32(reply + 20, ADSP::MatrixMap);
    assert(route.receive(epoch - 1, reply, 28) == Result::Stale);
    assert(route.receive(epoch, reply, 28) == Result::OK);
    assert(!route.confirmed(epoch, 2) && !route.confirmed(epoch - 1, 1));
    assert(route.receive(epoch, reply, 28) == Result::Stale);
    assert(!stream.run(route, packet, 31) && route.available(epoch, 1));
    assert(stream.run(route, packet, sizeof(packet)) == 32);
    assert(!route.available(epoch, 1) && route.confirmed(epoch, 1));
    assert(stream.write(route, 0, 4096, 2048, packet, sizeof(packet), &token) == LILAC_ASM_QUEUE_INVALID);
    ack(ADSP::Run);
    assert(stream.receive(epoch, reply, 28) == Result::OK);
    assert(stream.write(route, 0, 4096, 2048, packet, sizeof(packet), &token) == LILAC_ASM_QUEUE_RESERVED);
    assert(token == 4);
    assert(!stream.close(packet, sizeof(packet)) && !stream.unmap(packet, sizeof(packet)));
    ack(ADSP::Write);
    assert(stream.receive(epoch, reply, 28) == Result::Stale);
    response(reply, 36, port, token, ADSP::WriteDone);
    reply[1] = 0;
    ADSP::write64(reply + 20, address);
    ADSP::write32(reply + 28, 17);
    assert(stream.receive(epoch, reply, 36) == Result::OK);
    assert(!device.close(epoch, false, 2, packet, sizeof(packet)));
    assert(stream.close(packet, sizeof(packet)) == 20);
    assert(stream.write(route, 0, 4096, 2048, packet, sizeof(packet), &token) == LILAC_ASM_QUEUE_INVALID);
    assert(!stream.unmap(packet, sizeof(packet)));
    ack(ADSP::Close);
    assert(stream.receive(epoch, reply, 28) == Result::OK);
    assert(!stream.region_release_confirmed());
    assert(stream.unmap(packet, sizeof(packet)) == 24);
    response(reply, 28, 0, ADSP::read32(packet + 12), ADSP::BasicResponse);
    ADSP::write32(reply + 20, ADSP::Unmap);
    assert(stream.receive(epoch, reply, 28) == Result::OK && stream.region_release_confirmed());

    LilacAdmPlaybackRoute rejected;
    assert(!rejected.begin(epoch, 9, device, 7, packet, sizeof(packet)));
    assert(!rejected.begin(epoch, 1, device, 7, packet, 35));
    assert(rejected.begin(epoch, 1, device, 7, packet, sizeof(packet)) == 36);
    assert(ADSP::read16(packet + 32) == 3);
    response(reply, 28, 0, 7, ADSP::BasicResponse);
    ADSP::write32(reply + 20, ADSP::MatrixMap);
    assert(rejected.receive(epoch, reply, 28) == Result::Stale);
    reply[4] = reply[8] = ADSP::ADM;
    ADSP::write32(reply + 24, 1);
    assert(rejected.receive(epoch, reply, 28) == Result::IO);
    ADSP::write32(reply + 24, 0);
    assert(rejected.receive(epoch, reply, 28) == Result::Quarantined);
    assert(!rejected.confirmed(epoch, 1));

    assert(!device.close(epoch, true, 1, packet, sizeof(packet)));
    assert(device.close(epoch, true, 2, packet, sizeof(packet)) == 20);
    assert(ADSP::read16(packet + 6) == afe_port && ADSP::read16(packet + 10) == 3);
    assert(ADSP::read32(packet + 16) == LilacAdmPlaybackDevice::CloseV5);
    assert(!route.confirmed(epoch, 1));
    response(reply, 28, afe_port, 2, ADSP::BasicResponse);
    reply[4] = reply[8] = ADSP::ADM;
    ADSP::write32(reply + 20, LilacAdmPlaybackDevice::CloseV5);
    assert(device.receive(epoch, reply, 28) == Result::Stale);
    ADSP::write16(reply + 6, 3);
    assert(device.receive(epoch, reply, 28) == Result::OK);
    assert(device.state() == LilacAdmPlaybackDevice::State::Closed);

    LilacAsmPcmStream failed(epoch, port);
    assert(failed.open(topology, packet, sizeof(packet)) == 36);
    ack(ADSP::OpenWrite, 1);
    assert(failed.receive(epoch, reply, 28) == Result::IO);
    ack(ADSP::OpenWrite);
    assert(failed.receive(epoch, reply, 28) == Result::Quarantined);
    assert(!failed.region_release_confirmed() && !failed.open(topology, packet, sizeof(packet)));

    assert(device.open(epoch, afe_port, topology, 3, packet, sizeof(packet)) == 48);
    response(reply, 28, afe_port, 3, LilacAdmPlaybackDevice::OpenResponseV5);
    reply[4] = reply[8] = ADSP::ADM;
    ADSP::write16(reply + 24, 255);
    assert(device.receive(epoch, reply, 28) == Result::IO);
    ADSP::write16(reply + 24, 3);
    assert(device.receive(epoch, reply, 28) == Result::Quarantined);
    assert(device.state() == LilacAdmPlaybackDevice::State::Uncertain);
}
