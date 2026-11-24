#ifndef LILAC_BACKLIGHT_PROPERTIES_H
#define LILAC_BACKLIGHT_PROPERTIES_H

#include <stdint.h>

#define LILAC_BRIGHTNESS_KEY "brightness"
#define LILAC_FADE_KEY "FadePeriod"
#define LILAC_LEVEL_KEY "backlight-level"
#define LILAC_PROPERTY_MAX 0x7fffffffu

struct lilac_backlight_range {
    uint32_t minimum, maximum, value;
};

struct lilac_backlight_request {
    uint64_t brightness, fade, level;
    bool has_level;
};

static inline bool
lilac_backlight_range_valid(const struct lilac_backlight_range &range)
{
    return range.minimum < range.maximum && range.maximum <= LILAC_PROPERTY_MAX &&
        range.value >= range.minimum && range.value <= range.maximum;
}

static inline bool
lilac_backlight_request_valid(const struct lilac_backlight_range &range,
    const struct lilac_backlight_request &request)
{
    return lilac_backlight_range_valid(range) &&
        (request.brightness == 0 ||
         (request.brightness >= range.minimum && request.brightness <= range.maximum)) &&
        request.fade <= LILAC_PROPERTY_MAX &&
        (!request.has_level || request.level <= LILAC_PROPERTY_MAX);
}

#endif
