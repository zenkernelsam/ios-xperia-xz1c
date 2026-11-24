#include <assert.h>
#include <string.h>

#include "../../xnu/support/LilacAsmPcm.h"

int main(void)
{
    const struct lilac_asm_pcm_buffer buffer = {UINT64_C(0x123456780), 0x11223344, 4096};
    const uint8_t expected[32] = {
        0x80, 0x67, 0x45, 0x23, 1, 0, 0, 0, 0x44, 0x33, 0x22, 0x11,
        0, 8, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    uint8_t output[34];
    memset(output, 0xa5, sizeof(output));
    assert(lilac_asm_pcm_write_payload(output + 1, 32, &buffer, 2048, 7) == 32);
    assert(!memcmp(output + 1, expected, sizeof(expected)));
    assert(output[0] == 0xa5 && output[33] == 0xa5);

    const struct lilac_asm_pcm_buffer bad[] = {
        {1, 1, 4}, {0x1000, 0, 4}, {0x1000, 1, 3}, {(UINT64_C(1) << 36) - 32, 1, 36}
    };
    for (unsigned index = 0; index < sizeof(bad) / sizeof(bad[0]); ++index) {
        assert(!lilac_asm_pcm_write_payload(output + 1, 32, &bad[index], 4, 0));
    }
    assert(!lilac_asm_pcm_write_payload(output + 1, 31, &buffer, 4, 0));
    assert(!lilac_asm_pcm_write_payload(output + 1, 32, &buffer, 0, 0));
    assert(!lilac_asm_pcm_write_payload(output + 1, 32, &buffer, 4097, 0));
    assert(!lilac_asm_pcm_write_payload(output + 1, 32, &buffer, 4100, 0));
    assert(!memcmp(output + 1, expected, sizeof(expected)));

    uint8_t reply[16] = {0x80, 0x67, 0x45, 0x23, 1, 0, 0, 0, 0x44, 0x33, 0x22, 0x11};
    assert(lilac_asm_pcm_write_done(reply, sizeof(reply), &buffer) == LILAC_ASM_CONSUMED);
    for (unsigned index = 0; index < 12; ++index) {
        reply[index] ^= 1;
        assert(lilac_asm_pcm_write_done(reply, sizeof(reply), &buffer) == LILAC_ASM_UNMATCHED);
        reply[index] ^= 1;
    }
    assert(lilac_asm_pcm_write_done(reply, 15, &buffer) == LILAC_ASM_UNMATCHED);
    assert(lilac_asm_pcm_write_done(reply, 17, &buffer) == LILAC_ASM_UNMATCHED);
    reply[12] = 1;
    assert(lilac_asm_pcm_write_done(reply, sizeof(reply), &buffer) == LILAC_ASM_DSP_ERROR);
    const struct lilac_asm_pcm_submission submission = {buffer, 2, 0, 0x101};
    const struct lilac_asm_pcm_event done = {2, 0x10d99, 0, 0x101, 0x101, 7, 0, reply, sizeof(reply)};
    assert(lilac_asm_pcm_complete(&submission, &done) == LILAC_ASM_DSP_ERROR);
    reply[12] = 0;
    assert(lilac_asm_pcm_complete(&submission, &done) == LILAC_ASM_CONSUMED);
    const struct lilac_asm_pcm_event foreign[] = {
        {1, 0x10d99, 0, 0x101, 0x101, 7, 0, reply, sizeof(reply)},
        {2, 0x100be, 0, 0x101, 0x101, 7, 0, reply, sizeof(reply)},
        {2, 0x10d99, 1, 0x101, 0x101, 7, 0, reply, sizeof(reply)},
        {2, 0x10d99, 0, 0x102, 0x101, 7, 0, reply, sizeof(reply)},
        {2, 0x10d99, 0, 0x101, 0x102, 7, 0, reply, sizeof(reply)},
        {2, 0x10d99, 0, 0x101, 0x101, 4, 0, reply, sizeof(reply)},
        {2, 0x10d99, 0, 0x101, 0x101, 7, 1, reply, sizeof(reply)}
    };
    for (unsigned index = 0; index < sizeof(foreign) / sizeof(foreign[0]); ++index) {
        assert(lilac_asm_pcm_complete(&submission, &foreign[index]) == LILAC_ASM_UNMATCHED);
    }
}
