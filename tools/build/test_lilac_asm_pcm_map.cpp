#include <cassert>
#include <cstring>

#include "../../xnu/support/LilacAsmPcmMap.h"

static void reply(uint8_t *packet, size_t bytes, uint32_t token, uint32_t opcode)
{
    std::memset(packet, 0, bytes);
    ADSP::write16(packet, 0x150);
    ADSP::write16(packet + 2, bytes);
    packet[4] = packet[8] = ADSP::ASM;
    packet[5] = 4;
    packet[9] = 5;
    ADSP::write32(packet + 12, token);
    ADSP::write32(packet + 16, opcode);
}

int main()
{
    using ADSP::Result;
    using State = LilacAsmPcmMap::State;
    lilac_asm_pcm_queue queue{};
    LilacAsmPcmMap map;
    uint8_t request[52]{}, response[28]{};
    uint32_t token = 0;
    constexpr uint64_t address = 0x123456000;
    assert(lilac_asm_pcm_queue_open(&queue, 9, 0x101));
    assert(!map.map(queue, address + 32, 8192, request, sizeof(request)));
    assert(!map.map(queue, address, 4097, request, sizeof(request)));
    assert(!map.map(queue, (UINT64_C(1) << 36) - 4096, 8192, request, sizeof(request)));
    assert(!map.map(queue, address, 8192, request, 39));
    assert(map.state() == State::Empty);
    assert(map.map(queue, address, 8192, request, sizeof(request)) == 40);
    const uint8_t expected[] = {
        0x50, 2, 40, 0, 7, 5, 0, 0, 7, 4, 0, 0, 1, 0, 0, 0, 0x92, 0x0d, 1, 0,
        3, 0, 1, 0, 0, 0, 0, 0, 0, 0x60, 0x45, 0x23, 1, 0, 0, 0, 0, 0x20, 0, 0
    };
    assert(!std::memcmp(request, expected, sizeof(expected)));
    assert(map.write(0, 4096, 2048, request, sizeof(request), &token) == LILAC_ASM_QUEUE_INVALID);
    reply(response, 24, 1, ADSP::Accepted);
    assert(map.receive(9, response, 24) == Result::NotReady);
    reply(response, 28, 1, ADSP::BasicResponse);
    ADSP::write32(response + 20, ADSP::Map);
    assert(map.receive(9, response, 28) == Result::NotReady);
    reply(response, 24, 1, ADSP::MapResponse);
    ADSP::write32(response + 20, 0x11223344);
    assert(map.receive(8, response, 24) == Result::Stale);
    assert(map.receive(9, response, 23) == Result::Invalid);
    response[5] = 5;
    assert(map.receive(9, response, 24) == Result::Invalid);
    response[5] = 4;
    assert(map.receive(9, response, 24) == Result::OK);
    assert(map.receive(9, response, 24) == Result::Stale);
    assert(map.write(4096, 4097, 2048, request, sizeof(request), &token) == LILAC_ASM_QUEUE_INVALID);
    assert(map.write(4096, 4096, 2048, request, sizeof(request), &token) == LILAC_ASM_QUEUE_RESERVED);
    assert(Baseband::read64(request + 20) == address + 4096);
    assert(ADSP::read32(request + 28) == 0x11223344);
    assert(!map.unmap(request, sizeof(request)));
    uint8_t done[36];
    reply(done, sizeof(done), token, ADSP::WriteDone);
    done[1] = 0;
    ADSP::write16(done + 6, 0x101);
    ADSP::write16(done + 10, 0x101);
    ADSP::write64(done + 20, address + 4096);
    ADSP::write32(done + 28, 0x11223344);
    assert(lilac_asm_pcm_apr_receive(&queue, 9, done, sizeof(done)) == LILAC_ASM_QUEUE_RELEASED);
    assert(map.unmap(request, sizeof(request)) == 24);
    assert(ADSP::read32(request + 12) == 2 && ADSP::read32(request + 20) == 0x11223344);
    assert(map.write(0, 4096, 2048, request, sizeof(request), &token) == LILAC_ASM_QUEUE_INVALID);
    reply(response, 28, 2, ADSP::BasicResponse);
    ADSP::write32(response + 20, ADSP::Map);
    assert(map.receive(9, response, 28) == Result::Invalid && map.state() == State::Unmapping);
    ADSP::write32(response + 20, ADSP::Unmap);
    assert(map.receive(9, response, 28) == Result::OK && map.state() == State::Empty);
    assert(map.map(queue, address, 8192, request, sizeof(request)) == 40);
    assert(ADSP::read32(request + 12) == 3);
    map.uncertain();
    reply(response, 24, 3, ADSP::MapResponse);
    ADSP::write32(response + 20, 7);
    assert(map.receive(9, response, 24) == Result::Quarantined);
    assert(!map.unmap(request, sizeof(request)) && queue.quarantined);

    lilac_asm_pcm_queue faulty{};
    LilacAsmPcmMap malformed;
    assert(lilac_asm_pcm_queue_open(&faulty, 10, 0x101));
    assert(malformed.map(faulty, address, 4096, request, sizeof(request)) == 40);
    reply(response, 24, 1, ADSP::MapResponse);
    assert(malformed.receive(10, response, 24) == Result::Invalid);
    assert(malformed.state() == State::Uncertain && faulty.quarantined);

    lilac_asm_pcm_queue rejected{};
    LilacAsmPcmMap failed_unmap;
    assert(lilac_asm_pcm_queue_open(&rejected, 11, 0x101));
    assert(failed_unmap.map(rejected, address, 4096, request, sizeof(request)) == 40);
    reply(response, 24, 1, ADSP::MapResponse);
    ADSP::write32(response + 20, 7);
    assert(failed_unmap.receive(11, response, 24) == Result::OK);
    assert(failed_unmap.unmap(request, sizeof(request)) == 24);
    reply(response, 28, 2, ADSP::BasicResponse);
    ADSP::write32(response + 20, ADSP::Unmap);
    ADSP::write32(response + 24, 1);
    assert(failed_unmap.receive(11, response, 28) == Result::IO);
    ADSP::write32(response + 24, 0);
    assert(failed_unmap.receive(11, response, 28) == Result::Quarantined);
    assert(rejected.quarantined && failed_unmap.state() == State::Uncertain);
}
