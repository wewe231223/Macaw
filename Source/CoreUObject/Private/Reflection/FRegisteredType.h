#pragma once

#include "FReflectionDefinition.h"

struct FRegisteredType {
    FReflectionDefinition mDefinition{};
    std::unique_ptr<UStruct> mInformation{};
};
