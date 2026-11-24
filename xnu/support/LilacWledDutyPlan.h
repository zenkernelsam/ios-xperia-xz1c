#ifndef LILAC_WLED_DUTY_PLAN_H
#define LILAC_WLED_DUTY_PLAN_H

#include "LilacWledSnapshot.h"

enum lilac_wled_duty_result {
    LILAC_DUTY_REFUSED,
    LILAC_DUTY_UNCHANGED,
    LILAC_DUTY_PLANNED
};

struct lilac_wled_duty_step {
    uint8_t sink_offset;
    uint8_t value;
    uint16_t wait_after_us;
};

struct lilac_wled_duty_plan {
    enum lilac_wled_duty_result result;
    uint16_t target;
    unsigned count;
    struct lilac_wled_duty_step steps[6];
};

static inline struct lilac_wled_duty_plan
lilac_wled_plan_duty(const struct lilac_wled_snapshot *snapshot, unsigned requested,
    unsigned verified_ceiling)
{
    struct lilac_wled_duty_plan plan = {LILAC_DUTY_REFUSED, 0, 0, {{0, 0, 0}}};
    const unsigned minimum = (4095u * 4u) / 1000u;
    if (!snapshot || snapshot->state != LILAC_WLED_CONFIGURED ||
        snapshot->faults || snapshot->module_enabled != 1 ||
        snapshot->sink_mask != 3 || snapshot->modulator_mask != 3 ||
        snapshot->duty[0] != snapshot->duty[1] || snapshot->duty[0] < minimum ||
        snapshot->duty[0] > verified_ceiling || verified_ceiling > 4095 ||
        !requested || requested > verified_ceiling) {
        return plan;
    }

    plan.target = (uint16_t)(requested < minimum ? minimum : requested);
    if (plan.target == snapshot->duty[0]) {
        plan.result = LILAC_DUTY_UNCHANGED;
        return plan;
    }

    const uint8_t low = (uint8_t)plan.target;
    const uint8_t high = (uint8_t)(plan.target >> 8);
    const struct lilac_wled_duty_plan update = {
        LILAC_DUTY_PLANNED, plan.target, 6,
        {{0x57, low, 0}, {0x58, high, 0}, {0x67, low, 0}, {0x68, high, 0},
         {0x47, 0x0f, 1000}, {0x47, 0, 0}},
    };
    return update;
}

#endif
