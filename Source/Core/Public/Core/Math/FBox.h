#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

struct FRay;
struct FMatrix;

struct alignas(16) FBox {
public:
    ReflectTypes(FBox);

public:
    FBox() = default;
    FBox(const FVector& Minimum, const FVector& Maximum);
    ~FBox() = default;
    FBox(const FBox& Other) = default;
    FBox& operator=(const FBox& Other) = default;
    FBox(FBox&& Other) noexcept = default;
    FBox& operator=(FBox&& Other) noexcept = default;

public:
    ReflectStruct(FBox);

    bool Intersects(const FRay& Ray, float MaximumDistance) const;
    FBox Transformed(const FMatrix& Matrix) const;

private:
    ReflectMembers();

public:
    FVector mMinimum{};
    ReflectProperty(mMinimum, "Minimum", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    FVector mMaximum{};
    ReflectProperty(mMaximum, "Maximum", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
