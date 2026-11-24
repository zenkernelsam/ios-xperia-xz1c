#ifndef LILAC_BACKLIGHT_SERVICE_H
#define LILAC_BACKLIGHT_SERVICE_H

#include <IOKit/IOService.h>

#include "LilacBacklightProperties.h"

LIBKERN_RETURNS_RETAINED IOService *
lilac_backlight_create_service(IOService *provider, const struct lilac_backlight_range &range);

#endif
