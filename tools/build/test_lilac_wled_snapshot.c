#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../xnu/support/LilacWledSnapshot.h"

struct fixture {
    uint8_t bytes[9];
    unsigned calls;
    unsigned fail_at;
    unsigned change_at;
};

static int stock_read(void *context, unsigned ppid, unsigned offset, uint8_t *value)
{
    static const unsigned expected[] = {0xd808, 0xd846, 0xd946, 0xd950, 0xd960,
                                       0xd957, 0xd958, 0xd967, 0xd968};
    struct fixture *fixture = context;
    unsigned index = fixture->calls++ % 9;
    assert(ppid >> 8 == 3);
    assert(((ppid & 255u) << 8 | offset) == expected[index]);
    if (fixture->calls == fixture->fail_at) {
        return -1;
    }
    *value = fixture->bytes[index] ^ (fixture->calls == fixture->change_at ? 1u : 0u);
    return 0;
}

int main(void)
{
    const struct fixture active = {{0x20, 0x80, 0x30, 0x80, 0x80, 0xcd, 0xfa, 0xcd, 0xba}, 0, 0, 0};
    struct fixture fixture = active;
    struct lilac_wled_snapshot result = lilac_wled_capture(stock_read, &fixture);
    assert(result.state == LILAC_WLED_CONFIGURED && fixture.calls == 18);
    assert(result.duty[0] == 0xacd && result.duty[1] == result.duty[0]);
    assert(result.module_enabled == 1 && result.sink_mask == 3 && result.modulator_mask == 3 && !result.faults);

    fixture = active;
    fixture.bytes[1] = 0;
    assert(lilac_wled_capture(stock_read, &fixture).state == LILAC_WLED_DISABLED);
    for (unsigned fault = 1; fault <= 4; fault <<= 1) {
        fixture = active;
        fixture.bytes[0] |= fault;
        fixture.bytes[1] = 0;
        result = lilac_wled_capture(stock_read, &fixture);
        assert(result.state == LILAC_WLED_FAULT && result.faults == fault);
    }

    fixture = active;
    fixture.bytes[2] = 0x70;
    assert(lilac_wled_capture(stock_read, &fixture).state == LILAC_WLED_PARTIAL);
    fixture = active;
    fixture.bytes[4] = 0;
    assert(lilac_wled_capture(stock_read, &fixture).state == LILAC_WLED_PARTIAL);
    fixture = active;
    fixture.bytes[8] ^= 1;
    assert(lilac_wled_capture(stock_read, &fixture).state == LILAC_WLED_PARTIAL);
    fixture = active;
    memset(fixture.bytes + 5, 0, 4);
    assert(lilac_wled_capture(stock_read, &fixture).state == LILAC_WLED_ZERO_DUTY);

    for (unsigned call = 1; call <= 18; ++call) {
        fixture = active;
        fixture.fail_at = call;
        result = lilac_wled_capture(stock_read, &fixture);
        assert(result.state == LILAC_WLED_UNAVAILABLE && fixture.calls == call);
        assert(!result.duty[0] && !result.duty[1]);
    }
    for (unsigned call = 10; call <= 18; ++call) {
        fixture = active;
        fixture.change_at = call;
        result = lilac_wled_capture(stock_read, &fixture);
        assert(result.state == LILAC_WLED_CHANGED && fixture.calls == call);
        assert(!result.duty[0] && !result.duty[1]);
    }
    assert(lilac_wled_capture(0, 0).state == LILAC_WLED_UNAVAILABLE);
    puts("PASS: fixed Lilac WLED registers, stable duty, faults, partial state and interrupted reads");
}
