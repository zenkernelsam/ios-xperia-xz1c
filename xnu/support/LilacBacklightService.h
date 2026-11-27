#ifndef LILAC_BACKLIGHT_SERVICE_H
#define LILAC_BACKLIGHT_SERVICE_H

#include <IOKit/IOService.h>

#include "LilacBacklightProperties.h"
#include "LilacBacklightDutyBackend.h"

LIBKERN_RETURNS_RETAINED IOService *
lilac_backlight_create_service(IOService *provider, const struct lilac_backlight_range &range);

/* The owner retains and serializes this stable backend until service detach.
 * Zero/fade/auxiliary-level operations remain explicit unsupported paths. */
LIBKERN_RETURNS_RETAINED IOService *
lilac_backlight_create_service_with_duty_backend(IOService *provider,
    const struct lilac_backlight_range &range,struct lilac_backlight_duty_backend *backend);

#endif
