#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Base/FName.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "RenderCore/FUVTransform.generated.h"

USTRUCT()
struct FUVTransform {
public:
    GENERATED_TYPES(FUVTransform)

public:
    GENERATED_BODY(FUVTransform)

public:
    UPROPERTY(Edit, Serialize, Name = "UVScale", DisplayName = "UV Scale", Speed = 0.01f, Min = -100.0f, Max = 100.0f)
    FVector2 mScale{1.0f, 1.0f};
    UPROPERTY(Edit, Serialize, Name = "UVOffset", DisplayName = "UV Offset", Speed = 0.01f, Min = -100.0f, Max = 100.0f)
    FVector2 mOffset{};
};

struct FPrimitiveUVTransform {
    FName mMaterialGroup{};
    FUVTransform mTransform{};
    FObjectHandle mSourceHandle{};
};

static_assert(sizeof(FUVTransform) == 16);
