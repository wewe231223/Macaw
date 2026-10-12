#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Base/FName.h"
#include "CoreUObject/FObjectHandle.h"

struct FUVTransform {
    FVector2 mScale{1.0f, 1.0f};
    FVector2 mOffset{};
};

struct FPrimitiveUVTransform {
    FName mMaterialGroup{};
    FUVTransform mTransform{};
    FObjectHandle mSourceHandle{};
};

static_assert(sizeof(FUVTransform) == 16);
