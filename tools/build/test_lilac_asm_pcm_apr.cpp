#include <cassert>
#include <cstring>

#include "../../xnu/support/LilacAsmPcmApr.h"

int main()
{
    lilac_asm_pcm_queue queue{};
    assert(lilac_asm_pcm_queue_open(&queue, 8, 0x101));
    const lilac_asm_pcm_buffer buffer{UINT64_C(0x123456780), 0x11223344, 4096};
    uint8_t bytes[54]{};
    uint32_t token = 99;
    assert(lilac_asm_pcm_apr_build_write(&queue, &buffer, 2048, bytes + 1, 51, &token) == LILAC_ASM_QUEUE_INVALID);
    assert(queue.last_token == 0 && token == 99);
    bytes[0] = bytes[53] = 0xa5;
    assert(lilac_asm_pcm_apr_build_write(&queue, &buffer, 2048, bytes + 1, 52, &token) == LILAC_ASM_QUEUE_RESERVED);
    assert(token == 1 && bytes[0] == 0xa5 && bytes[53] == 0xa5);
    const uint8_t expected[52] = {
        0x50, 2, 52, 0, 7, 5, 1, 1, 7, 4, 1, 1, 1, 0, 0, 0, 0xab, 0x0d, 1, 0,
        0x80, 0x67, 0x45, 0x23, 1, 0, 0, 0, 0x44, 0x33, 0x22, 0x11,
        0, 8, 0, 0, 1
    };
    assert(!std::memcmp(bytes + 1, expected, sizeof(expected)));

    uint8_t reply[36] = {
        0x50, 0, 36, 0, 7, 4, 1, 1, 7, 5, 1, 1, 1, 0, 0, 0, 0x99, 0x0d, 1, 0,
        0x80, 0x67, 0x45, 0x23, 1, 0, 0, 0, 0x44, 0x33, 0x22, 0x11
    };
    for (size_t size = 0; size < sizeof(reply); ++size) {
        assert(lilac_asm_pcm_apr_receive(&queue, 8, reply, size) == LILAC_ASM_QUEUE_INVALID);
    }
    assert(lilac_asm_pcm_apr_receive(&queue, 7, reply, sizeof(reply)) == LILAC_ASM_QUEUE_STALE);
    const unsigned fields[] = {1, 5, 6, 8, 9, 10, 12, 16, 20, 24, 28};
    for (unsigned index : fields) {
        reply[index] ^= 1;
        const auto result = lilac_asm_pcm_apr_receive(&queue, 8, reply, sizeof(reply));
        assert(result == LILAC_ASM_QUEUE_STALE || result == LILAC_ASM_QUEUE_INVALID);
        assert(queue.pending[0].epoch == 8);
        reply[index] ^= 1;
    }
    reply[16] = 0xbe;
    reply[17] = 0;
    assert(lilac_asm_pcm_apr_receive(&queue, 8, reply, sizeof(reply)) == LILAC_ASM_QUEUE_STALE);
    reply[16] = 0x99;
    reply[17] = 0x0d;
    assert(lilac_asm_pcm_apr_receive(&queue, 8, reply, sizeof(reply)) == LILAC_ASM_QUEUE_RELEASED);
    assert(lilac_asm_pcm_apr_receive(&queue, 8, reply, sizeof(reply)) == LILAC_ASM_QUEUE_STALE);

    assert(lilac_asm_pcm_apr_build_write(&queue, &buffer, 2048, bytes + 1, 52, &token) == LILAC_ASM_QUEUE_RESERVED);
    assert(token == 2);
    assert(lilac_asm_pcm_apr_receive(&queue, 8, reply, sizeof(reply)) == LILAC_ASM_QUEUE_STALE);
    reply[12] = 2;
    reply[32] = 1;
    assert(lilac_asm_pcm_apr_receive(&queue, 8, reply, sizeof(reply)) == LILAC_ASM_QUEUE_QUARANTINED);
    reply[32] = 0;
    assert(lilac_asm_pcm_apr_receive(&queue, 8, reply, sizeof(reply)) == LILAC_ASM_QUEUE_QUARANTINED);
    assert(queue.pending[0].epoch == 8);
}
