#include <assert.h>
#include <stdio.h>

#include "../../xnu/support/LilacWledDutyPlan.h"

static void require_refused(const struct lilac_wled_snapshot *snapshot, unsigned requested, unsigned ceiling)
{
    const struct lilac_wled_duty_plan plan = lilac_wled_plan_duty(snapshot, requested, ceiling);
    assert(plan.result == LILAC_DUTY_REFUSED && plan.count == 0 && plan.target == 0);
}

int main(void)
{
    const struct lilac_wled_snapshot healthy = {LILAC_WLED_CONFIGURED, 0, 1, 3, 3, {564, 564}};
    const struct lilac_wled_duty_plan plan = lilac_wled_plan_duty(&healthy, 0xabc, 3000);
    assert(plan.result == LILAC_DUTY_PLANNED && plan.target == 0xabc && plan.count == 6);
    const struct lilac_wled_duty_step expected[6] = {
        {0x57, 0xbc, 0}, {0x58, 0x0a, 0}, {0x67, 0xbc, 0}, {0x68, 0x0a, 0},
        {0x47, 0x0f, 1000}, {0x47, 0, 0}
    };
    for (unsigned index = 0; index < 6; ++index) {
        assert(plan.steps[index].sink_offset == expected[index].sink_offset);
        assert(plan.steps[index].value == expected[index].value);
        assert(plan.steps[index].wait_after_us == expected[index].wait_after_us);
    }

    for (unsigned requested = 1; requested <= 4095; ++requested) {
        const struct lilac_wled_duty_plan trial = lilac_wled_plan_duty(&healthy, requested, 4095);
        assert(trial.target == (requested < 16 ? 16 : requested));
        if (requested == healthy.duty[0]) {
            assert(trial.result == LILAC_DUTY_UNCHANGED && trial.count == 0);
        } else {
            assert(trial.result == LILAC_DUTY_PLANNED && trial.count == 6);
            assert((unsigned)(trial.steps[0].value | (trial.steps[1].value << 8)) == trial.target);
            assert(trial.steps[2].value == trial.steps[0].value);
            assert(trial.steps[3].value == trial.steps[1].value);
        }
    }

    require_refused(NULL, 100, 1000);
    require_refused(&healthy, 0, 1000);
    require_refused(&healthy, 1001, 1000);
    require_refused(&healthy, 100, 0);
    require_refused(&healthy, 100, 500);
    require_refused(&healthy, 100, 4096);
    for (unsigned state = LILAC_WLED_UNAVAILABLE; state < LILAC_WLED_CONFIGURED; ++state) {
        struct lilac_wled_snapshot bad = healthy;
        bad.state = (enum lilac_wled_state)state;
        require_refused(&bad, 100, 1000);
    }
    struct lilac_wled_snapshot bad = healthy;
    bad.faults = 1;
    require_refused(&bad, 100, 1000);
    bad = healthy;
    bad.module_enabled = 0;
    require_refused(&bad, 100, 1000);
    bad = healthy;
    bad.duty[1] = 1024;
    require_refused(&bad, 100, 1000);
    bad = healthy;
    bad.sink_mask = 7;
    require_refused(&bad, 100, 1000);
    bad = healthy;
    bad.modulator_mask = 1;
    require_refused(&bad, 100, 1000);
    bad = healthy;
    bad.duty[0] = bad.duty[1] = 15;
    require_refused(&bad, 100, 1000);
    puts("PASS: Lilac WLED duty plan order, sync delay, all levels and refusal boundaries");
}
