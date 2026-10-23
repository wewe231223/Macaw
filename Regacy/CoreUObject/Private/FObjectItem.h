#pragma once

#include "Core/CoreMinimal.h"

#include <cstdint>
#include "CoreUObject/UObject.h"

struct FObjectItem {
    UObject* mObject{nullptr};
    std::uint32_t mGeneration{1};
};
