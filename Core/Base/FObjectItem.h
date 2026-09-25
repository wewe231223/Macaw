#pragma once

#include <cstdint>

#include "UObject.h"

struct FObjectItem {
    UObject* mObject{nullptr};
    std::uint32_t mGeneration{1};
};
