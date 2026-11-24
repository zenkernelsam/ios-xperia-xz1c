#ifndef LILAC_WLED_SNAPSHOT_H
#define LILAC_WLED_SNAPSHOT_H

#include <stdint.h>

enum lilac_wled_state {
    LILAC_WLED_UNAVAILABLE,
    LILAC_WLED_CHANGED,
    LILAC_WLED_FAULT,
    LILAC_WLED_DISABLED,
    LILAC_WLED_PARTIAL,
    LILAC_WLED_ZERO_DUTY,
    LILAC_WLED_CONFIGURED
};

struct lilac_wled_snapshot {
    enum lilac_wled_state state;
    uint8_t faults;
    uint8_t module_enabled;
    uint8_t sink_mask;
    uint8_t modulator_mask;
    uint16_t duty[2];
};

typedef int (*lilac_wled_read)(void *, unsigned, unsigned, uint8_t *);

static inline struct lilac_wled_snapshot
lilac_wled_capture(lilac_wled_read read, void *context)
{
    static const uint8_t control_ppid_high = 3;
    static const uint8_t offsets[] = {0x08, 0x46, 0x46, 0x50, 0x60, 0x57, 0x58, 0x67, 0x68};
    struct lilac_wled_snapshot result = { LILAC_WLED_UNAVAILABLE, 0, 0, 0, 0, {0, 0} };
    uint8_t first[sizeof(offsets)], after;
    if (!read) {
        return result;
    }
    for (unsigned pass = 0; pass < 2; ++pass) {
        for (unsigned index = 0; index < sizeof(offsets); ++index) {
            unsigned ppid = (control_ppid_high << 8) | (index < 2 ? 0xd8u : 0xd9u);
            if (read(context, ppid, offsets[index], &after) != 0) {
                return result;
            }
            if (pass == 0) {
                first[index] = after;
            } else if (first[index] != after) {
                result.state = LILAC_WLED_CHANGED;
                return result;
            }
        }
    }
    result.faults = first[0] & 7u;
    result.module_enabled = (first[1] >> 7) & 1u;
    result.sink_mask = first[2] >> 4;
    result.modulator_mask = ((first[3] >> 7) & 1u) | ((first[4] >> 6) & 2u);
    result.duty[0] = (uint16_t)(((first[6] & 15u) << 8) | first[5]);
    result.duty[1] = (uint16_t)(((first[8] & 15u) << 8) | first[7]);
    if (result.faults) {
        result.state = LILAC_WLED_FAULT;
    } else if (!result.module_enabled) {
        result.state = LILAC_WLED_DISABLED;
    } else if (result.sink_mask != 3 || result.modulator_mask != 3 || result.duty[0] != result.duty[1]) {
        result.state = LILAC_WLED_PARTIAL;
    } else {
        result.state = result.duty[0] ? LILAC_WLED_CONFIGURED : LILAC_WLED_ZERO_DUTY;
    }
    return result;
}

#endif
