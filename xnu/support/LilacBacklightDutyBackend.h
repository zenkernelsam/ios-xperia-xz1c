/* SPDX-License-Identifier: BSD-2-Clause
 * Property-range to native nonzero duty adapter. Uses the existing immutable
 * WLED transaction contract; no mapping, allocation, bus init or power writes.
 * The owner supplies a proven range/ceiling and serial, non-reentrant lifetime.
 */
#ifndef LILAC_BACKLIGHT_DUTY_BACKEND_H
#define LILAC_BACKLIGHT_DUTY_BACKEND_H
#include "LilacBacklightProperties.h"
#include "LilacWledDutyTransaction.h"

enum lilac_backlight_apply_result {
    LILAC_BACKLIGHT_BAD_REQUEST,
    LILAC_BACKLIGHT_NOT_READY,
    LILAC_BACKLIGHT_POWER_PATH_REQUIRED,
    LILAC_BACKLIGHT_FADE_PATH_REQUIRED,
    LILAC_BACKLIGHT_LEVEL_PATH_REQUIRED,
    LILAC_BACKLIGHT_BUSY,
    LILAC_BACKLIGHT_DUTY_UNCERTAIN,
    LILAC_BACKLIGHT_DUTY_VERIFIED
};

struct lilac_backlight_duty_backend {
    struct lilac_wled_duty_io io;
    void *context;
    int (*ready)(void *, uint64_t *epoch);
    uint64_t verified_epoch;
    uint16_t verified_ceiling;
    struct lilac_wled_transaction transaction;
    bool busy;
};

static inline bool lilac_backlight_backend_valid(const lilac_backlight_duty_backend &backend)
{
    return backend.io.read && backend.io.write_sink && backend.io.wait_us && backend.ready && backend.verified_epoch &&
        backend.verified_ceiling >= (4095u * 4u) / 1000u && backend.verified_ceiling <= 4095u;
}

static inline enum lilac_backlight_apply_result
lilac_backlight_apply_nonzero(lilac_backlight_duty_backend *backend,
    const lilac_backlight_range &range,const lilac_backlight_request &request)
{
    if (!backend || !lilac_backlight_backend_valid(*backend) ||
        !lilac_backlight_request_valid(range,request)) return LILAC_BACKLIGHT_BAD_REQUEST;
    if (backend->busy) return LILAC_BACKLIGHT_BUSY;
    if (backend->transaction.uncertain) return LILAC_BACKLIGHT_DUTY_UNCERTAIN;
    if (!request.brightness) return LILAC_BACKLIGHT_POWER_PATH_REQUIRED;
    if (request.fade) return LILAC_BACKLIGHT_FADE_PATH_REQUIRED;
    if (request.has_level) return LILAC_BACKLIGHT_LEVEL_PATH_REQUIRED;
    backend->busy=true;
    enum lilac_backlight_apply_result result=LILAC_BACKLIGHT_NOT_READY;
    uint64_t epoch=0;
    if (backend->ready(backend->context,&epoch)==1 && epoch==backend->verified_epoch) {
        // CBDisplay's integer is normalized through the advertised min/max.
        // Use integer ceiling, then the exact stock 0.4% minimum. It is a
        // native duty mapping, not an optical brightness or dimming curve.
        const uint64_t span=uint64_t(range.maximum)-range.minimum;
        const uint64_t numerator=(request.brightness-range.minimum)*backend->verified_ceiling;
        unsigned duty=unsigned((numerator+span-1)/span);
        const unsigned minimum=(4095u*4u)/1000u;
        if (duty<minimum) duty=minimum;
        const auto applied=lilac_wled_transact_duty(&backend->transaction,&backend->io,
            backend->context,duty,backend->verified_ceiling);
        if (applied==LILAC_DUTY_TRANSACTION_VERIFIED || applied==LILAC_DUTY_TRANSACTION_UNCHANGED) {
            epoch=0;
            if (backend->ready(backend->context,&epoch)==1 && epoch==backend->verified_epoch)
                result=LILAC_BACKLIGHT_DUTY_VERIFIED;
            else {
                backend->transaction.uncertain=1;
                result=LILAC_BACKLIGHT_DUTY_UNCERTAIN;
            }
        }
        else if (applied==LILAC_DUTY_TRANSACTION_UNCERTAIN || applied==LILAC_DUTY_TRANSACTION_LOCKED)
            result=LILAC_BACKLIGHT_DUTY_UNCERTAIN;
    }
    backend->busy=false;
    return result;
}
#endif
