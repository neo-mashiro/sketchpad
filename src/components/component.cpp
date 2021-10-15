#include "pch.h"

#include "components/component.h"
#include "utils/math.h"

namespace components {

    Component::Component()
        : uuid(RandomUInt64()), enabled(true) {}

    uint64_t Component::GetUUID() const {
        return uuid;
    }

    void Component::Enable() {
        enabled = true;
    }

    void Component::Disable() {
        enabled = false;
    }

    bool Component::Enabled() const {
        return enabled;
    }

}