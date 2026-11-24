#include <cassert>

#include "../../xnu/support/LilacBacklightProperties.h"

int main()
{
    const lilac_backlight_range range = {16, 4095, 1024};
    assert(lilac_backlight_range_valid(range));
    assert(!lilac_backlight_range_valid({16, 16, 16}));
    assert(!lilac_backlight_range_valid({16, 4095, 0}));
    assert(!lilac_backlight_range_valid({16, 4095, 4096}));
    assert(!lilac_backlight_range_valid({0, 0x80000000u, 1}));

    assert(lilac_backlight_request_valid(range, {0, 0, 0, false}));
    assert(lilac_backlight_request_valid(range, {16, 65536, 512, true}));
    assert(lilac_backlight_request_valid(range, {4095, 0, 0, false}));
    assert(!lilac_backlight_request_valid(range, {15, 0, 0, false}));
    assert(!lilac_backlight_request_valid(range, {4096, 0, 0, false}));
    assert(!lilac_backlight_request_valid(range, {(1ull << 32) | 1024, 0, 0, false}));
    assert(!lilac_backlight_request_valid(range, {1024, UINT64_MAX, 0, false}));
    assert(!lilac_backlight_request_valid(range, {1024, 65536, 0xffffffffu, true}));
}
