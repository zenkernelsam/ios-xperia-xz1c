#ifndef LILAC_WLED_SCREEN_PROBE_H
#define LILAC_WLED_SCREEN_PROBE_H

#include "LilacWledSnapshot.h"

extern "C" int sweet_spmi_read_ppid(unsigned, unsigned, uint8_t *);

static bool lilac_wled_screen_address_allowed(unsigned ppid, unsigned offset)
{
    static const uint8_t control_registers[] = {0x08, 0x46};
    static const uint8_t sink_registers[] = {0x46, 0x50, 0x60, 0x57, 0x58, 0x67, 0x68};
    const uint8_t *registers = nullptr;
    unsigned count = 0;
    if (ppid == 0x3d8u) {
        registers = control_registers;
        count = sizeof(control_registers);
    } else if (ppid == 0x3d9u) {
        registers = sink_registers;
        count = sizeof(sink_registers);
    }
    for (unsigned index = 0; index < count; ++index) {
        if (offset == registers[index]) {
            return true;
        }
    }
    return false;
}

static int lilac_wled_screen_read(void *, unsigned ppid, unsigned offset, uint8_t *value)
{
    if (!value || !vermeer_loop_live_ready() || !lilac_wled_screen_address_allowed(ppid, offset)) {
        return -1;
    }
    return sweet_spmi_read_ppid(ppid, offset, value);
}

static void lilac_wled_screen_probe(const char *token, uint32_t frame_id)
{
    if (frame_id != 1 || !vermeer_loop_live_ready()) {
        return;
    }
    const struct lilac_wled_snapshot snapshot = lilac_wled_capture(lilac_wled_screen_read, nullptr);
    if (!vermeer_loop_live_ready()) {
        return;
    }
    sweet_screen_record(
        "SWEET WLED SAMPLE token=%s id=%u state=%u fault=%u enabled=%u sinks=%u modulators=%u duty0=%u duty1=%u read_only=1",
        token, frame_id, (unsigned)snapshot.state, (unsigned)snapshot.faults,
        (unsigned)snapshot.module_enabled, (unsigned)snapshot.sink_mask,
        (unsigned)snapshot.modulator_mask, (unsigned)snapshot.duty[0], (unsigned)snapshot.duty[1]);
}

#endif
