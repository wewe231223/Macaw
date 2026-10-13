#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/CoreMinimal.h"
#include "Core/Base/FName.h"
#include "CoreUObject/FObjectHandle.h"

struct FUVTransform {
public:
    ReflectTypes(FUVTransform);

public:
    ReflectStruct(FUVTransform);

public:
    ReflectProperty(mScale, "UVScale", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mScale), ReflectEditor("", "UV Scale"), ReflectRange(-100.0f, 100.0f), ReflectSpeed(0.01f));
    FVector2 mScale{1.0f, 1.0f};
    ReflectProperty(mOffset, "UVOffset", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mOffset), ReflectEditor("", "UV Offset"), ReflectRange(-100.0f, 100.0f), ReflectSpeed(0.01f));
    FVector2 mOffset{};

    ReflectMembers();
};

struct FPrimitiveUVTransform {
    FName mMaterialGroup{};
    FUVTransform mTransform{};
    FObjectHandle mSourceHandle{};
};

static_assert(sizeof(FUVTransform) == 16);
