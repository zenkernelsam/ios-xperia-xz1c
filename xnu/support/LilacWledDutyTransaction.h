#ifndef LILAC_WLED_DUTY_TRANSACTION_H
#define LILAC_WLED_DUTY_TRANSACTION_H

#include "LilacWledDutyPlan.h"

enum lilac_wled_transaction_result {
    LILAC_DUTY_TRANSACTION_REFUSED,
    LILAC_DUTY_TRANSACTION_UNCHANGED,
    LILAC_DUTY_TRANSACTION_VERIFIED,
    LILAC_DUTY_TRANSACTION_UNCERTAIN,
    LILAC_DUTY_TRANSACTION_LOCKED
};

struct lilac_wled_transaction {
    uint8_t uncertain;
};

struct lilac_wled_duty_io {
    lilac_wled_read read;
    int (*write_sink)(void *, uint8_t, uint8_t);
    int (*wait_us)(void *, uint16_t);
};

static inline int
lilac_wled_sync_idle(lilac_wled_read read, void *context)
{
    uint8_t sync;
    return read(context, 0x3d9u, 0x47u, &sync) == 0 && sync == 0;
}

static inline enum lilac_wled_transaction_result
lilac_wled_transact_duty(struct lilac_wled_transaction *transaction, const struct lilac_wled_duty_io *io,
    void *context, unsigned requested, unsigned verified_ceiling)
{
    if (!transaction || !io || !io->read || !io->write_sink || !io->wait_us) {
        return LILAC_DUTY_TRANSACTION_REFUSED;
    }
    if (transaction->uncertain) {
        return LILAC_DUTY_TRANSACTION_LOCKED;
    }
    const struct lilac_wled_snapshot before = lilac_wled_capture(io->read, context);
    const struct lilac_wled_duty_plan plan = lilac_wled_plan_duty(&before, requested, verified_ceiling);
    if (plan.result == LILAC_DUTY_REFUSED || !lilac_wled_sync_idle(io->read, context)) {
        return LILAC_DUTY_TRANSACTION_REFUSED;
    }
    if (plan.result == LILAC_DUTY_UNCHANGED) {
        return LILAC_DUTY_TRANSACTION_UNCHANGED;
    }
    transaction->uncertain = 1;
    for (unsigned index = 0; index < plan.count; ++index) {
        const struct lilac_wled_duty_step *step = &plan.steps[index];
        if (io->write_sink(context, step->sink_offset, step->value) != 0 ||
            (step->wait_after_us && io->wait_us(context, step->wait_after_us) != 0)) {
            return LILAC_DUTY_TRANSACTION_UNCERTAIN;
        }
    }
    const struct lilac_wled_snapshot after = lilac_wled_capture(io->read, context);
    if (lilac_wled_plan_duty(&after, plan.target, verified_ceiling).result != LILAC_DUTY_UNCHANGED ||
        !lilac_wled_sync_idle(io->read, context)) {
        return LILAC_DUTY_TRANSACTION_UNCERTAIN;
    }
    transaction->uncertain = 0;
    return LILAC_DUTY_TRANSACTION_VERIFIED;
}

#endif
