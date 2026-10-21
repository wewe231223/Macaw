#pragma once

#include "CoreUObject/FObjectHandle.h"
#include "Core/Math/FVector.h"

struct FWorldHit {
    FObjectHandle mActor{};
    FObjectHandle mComponent{};

    FVector mPosition{};
    float mDistance{};
};
