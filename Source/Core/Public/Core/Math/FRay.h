#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

struct alignas(16) FRay {
public:
    ReflectTypes(FRay);

public:
    FRay() = default;
    FRay(const FVector& Origin, const FVector& Direction);
    ~FRay() = default;
    FRay(const FRay& Other) = default;
    FRay& operator=(const FRay& Other) = default;
    FRay(FRay&& Other) noexcept = default;
    FRay& operator=(FRay&& Other) noexcept = default;

private:
    ReflectMembers();

public:
    ReflectStruct(FRay);

    FVector mOrigin{};
    ReflectProperty(mOrigin, "Origin", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    FVector mDirection{1.0f, 0.0f, 0.0f};
    ReflectProperty(mDirection, "Direction", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
