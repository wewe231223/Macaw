#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FQuat.h"

struct alignas(16) FTransform {
public:
    ReflectTypes(FTransform);

public:
    FTransform() = default;
    FTransform(const FVector& Location, const FQuat& Rotation, const FVector& Scale);
    ~FTransform() = default;
    FTransform(const FTransform& Other) = default;
    FTransform& operator=(const FTransform& Other) = default;
    FTransform(FTransform&& Other) noexcept = default;
    FTransform& operator=(FTransform&& Other) noexcept = default;

public:
    ReflectStruct(FTransform);

    FTransform operator*(const FTransform& Other) const;
    FTransform& operator*=(const FTransform& Other);

    FVector TransformPosition(const FVector& Position) const;
    FVector TransformVector(const FVector& Vector) const;
    FVector InverseTransformPosition(const FVector& Position) const;
    FVector InverseTransformVector(const FVector& Vector) const;

    FVector TransformPositionNoScale(const FVector& Position) const;
    FVector TransformVectorNoScale(const FVector& Vector) const;
    FVector InverseTransformPositionNoScale(const FVector& Position) const;
    FVector InverseTransformVectorNoScale(const FVector& Vector) const;

    FTransform Blend(const FTransform& Other, float Alpha) const;
    bool NormalizeRotation();
    bool IsIdentity(float Tolerance = 0.0001f) const;
    bool Equals(const FTransform& Other, float Tolerance = 0.0001f) const;

    FRotator GetRotator() const;
    bool SetRotator(const FRotator& Rotation);

    FMatrix ToMatrix() const;
    bool IsValid() const;

private:
    ReflectMembers();

public:
    FVector mLocation{};
    ReflectProperty(mLocation, "Location", EPropertyFlags::Edit | EPropertyFlags::Serialize);


    FQuat mRotation{};
    ReflectAccessor(mRotation, GetRotator, SetRotator, "Rotation", EPropertyFlags::Edit | EPropertyFlags::Serialize);


    FVector mScale{1.0f, 1.0f, 1.0f};
    ReflectProperty(mScale, "Scale", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
