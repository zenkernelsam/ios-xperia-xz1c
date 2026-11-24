#include <IOKit/IOKitKeys.h>
#include <libkern/c++/OSArray.h>
#include <libkern/c++/OSDictionary.h>
#include <libkern/c++/OSNumber.h>
#include <libkern/c++/OSString.h>

#include "LilacBacklightService.h"

class AppleARMBacklight : public IOService {
    OSDeclareDefaultStructors(AppleARMBacklight);

public:
    bool initWithBrightnessRange(const struct lilac_backlight_range &range);
    IOReturn setProperties(OSObject *properties) APPLE_KEXT_OVERRIDE;

private:
    struct lilac_backlight_range fRange;
};

OSDefineMetaClassAndStructors(AppleARMBacklight, IOService);

static bool
lilac_set_display_number(OSDictionary *dictionary, const char *key, uint32_t value)
{
    auto number = OSNumber::withNumber(value, 32);
    if (!number) {
        return false;
    }
    bool added = dictionary->setObject(key, number);
    number->release();
    return added;
}

static bool
lilac_is_display_number(const OSNumber *number)
{
    return number && (number->numberOfBits() == 32 || number->numberOfBits() == 64);
}

bool
AppleARMBacklight::initWithBrightnessRange(const struct lilac_backlight_range &range)
{
    if (!lilac_backlight_range_valid(range) || !IOService::init()) {
        return false;
    }
    fRange = range;
    auto parameters = OSDictionary::withCapacity(1);
    auto brightness = OSDictionary::withCapacity(3);
    auto allowable = OSArray::withCapacity(3);
    bool ready = parameters && brightness && allowable;
    const char *const keys[] = { LILAC_BRIGHTNESS_KEY, LILAC_FADE_KEY, LILAC_LEVEL_KEY };
    for (const char *key : keys) {
        if (!ready) {
            break;
        }
        auto name = OSString::withCString(key);
        ready = name && allowable->setObject(name);
        if (name) {
            name->release();
        }
    }
    ready = ready &&
        lilac_set_display_number(brightness, "value", range.value) &&
        lilac_set_display_number(brightness, "min", range.minimum) &&
        lilac_set_display_number(brightness, "max", range.maximum) &&
        parameters->setObject(LILAC_BRIGHTNESS_KEY, brightness) &&
        setProperty(kIORegistryEntryAllowableSetPropertiesKey, allowable) &&
        setProperty("IODisplayParameters", parameters) &&
        setProperty("backlight-control", true);
    if (allowable) {
        allowable->release();
    }
    if (brightness) {
        brightness->release();
    }
    if (parameters) {
        parameters->release();
    }
    return ready;
}

IOReturn
AppleARMBacklight::setProperties(OSObject *properties)
{
    const auto dictionary = OSDynamicCast(OSDictionary, properties);
    if (!dictionary) {
        return kIOReturnBadArgument;
    }
    const auto brightness = OSDynamicCast(OSNumber, dictionary->getObject(LILAC_BRIGHTNESS_KEY));
    const auto fade = OSDynamicCast(OSNumber, dictionary->getObject(LILAC_FADE_KEY));
    const auto levelObject = dictionary->getObject(LILAC_LEVEL_KEY);
    const auto level = OSDynamicCast(OSNumber, levelObject);
    if (!lilac_is_display_number(brightness) || !lilac_is_display_number(fade) ||
        (levelObject && !lilac_is_display_number(level)) ||
        dictionary->getCount() != (level ? 3u : 2u)) {
        return kIOReturnBadArgument;
    }
    const struct lilac_backlight_request request = {
        brightness->unsigned64BitValue(), fade->unsigned64BitValue(),
        level ? level->unsigned64BitValue() : 0, level != nullptr,
    };
    return lilac_backlight_request_valid(fRange, request) ? kIOReturnNotReady : kIOReturnBadArgument;
}

IOService *
lilac_backlight_create_service(IOService *provider, const struct lilac_backlight_range &range)
{
    if (!provider || provider->isInactive() || !lilac_backlight_range_valid(range)) {
        return nullptr;
    }
    auto service = new AppleARMBacklight;
    if (!service) {
        return nullptr;
    }
    if (!service->initWithBrightnessRange(range) || !service->attach(provider)) {
        service->release();
        return nullptr;
    }
    if (!service->start(provider)) {
        service->detach(provider);
        service->release();
        return nullptr;
    }
    service->registerService();
    return service;
}
