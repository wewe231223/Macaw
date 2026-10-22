#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <xmmintrin.h>

struct FRotator;

#pragma warning(push)
#pragma warning(disable: 4201)

struct alignas(16) FVector {
private:
    friend struct FBox;
    friend struct FMatrix;
    friend struct FQuat;

public:
    ReflectTypes(FVector);

public:
    FVector() = default;
    FVector(float X, float Y, float Z);

private:
    explicit FVector(__m128 Value);

public:
    ~FVector() = default;
    FVector(const FVector& Other) = default;
    FVector& operator=(const FVector& Other) = default;
    FVector(FVector&& Other) noexcept = default;
    FVector& operator=(FVector&& Other) noexcept = default;

public:
    ReflectStruct(FVector);

    FVector operator+(const FVector& Other) const;
    FVector operator-(const FVector& Other) const;
    FVector operator-() const;
    FVector operator*(const FVector& Other) const;
    FVector operator/(const FVector& Other) const;
    FVector operator*(float Value) const;
    FVector operator/(float Value) const;
    FVector& operator+=(const FVector& Other);
    FVector& operator-=(const FVector& Other);
    FVector& operator*=(const FVector& Other);
    FVector& operator/=(const FVector& Other);
    FVector& operator*=(float Value);
    FVector& operator/=(float Value);
    bool operator==(const FVector& Other) const;

    float Dot(const FVector& Other) const;
    FVector Cross(const FVector& Other) const;
    FVector ProjectedOn(const FVector& Direction) const;
    FVector ProjectedOnPlane(const FVector& Normal) const;
    FVector Reflected(const FVector& Normal) const;
    FVector RotatedAngleAxis(float Degrees, const FVector& Axis) const;
    FRotator Rotation() const;

    float LengthSquared() const;
    float Length() const;
    FVector Normalized() const;
    bool Normalize();

    float DistanceSquared(const FVector& Other) const;
    float Distance(const FVector& Other) const;
    FVector Lerp(const FVector& Other, float Alpha) const;

    FVector Abs() const;
    FVector Min(const FVector& Other) const;
    FVector Max(const FVector& Other) const;
    FVector Clamped(const FVector& Minimum, const FVector& Maximum) const;
    FVector ClampedLength(float MaximumLength) const;

    bool IsNearlyZero(float Tolerance = 0.0001f) const;
    bool IsNormalized(float Tolerance = 0.0001f) const;

    bool IsFinite() const;
    bool Equals(const FVector& Other, float Tolerance = 0.0001f) const;

    std::array<float, 3> ToArray() const;

private:
    ReflectMembers();

public:
    union {
        __m128 mValue{};
        ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

        struct {
            float x;
            ReflectProperty(x, "X", EPropertyFlags::Edit | EPropertyFlags::Serialize);


            float y;
            ReflectProperty(y, "Y", EPropertyFlags::Edit | EPropertyFlags::Serialize);


            float z;
            ReflectProperty(z, "Z", EPropertyFlags::Edit | EPropertyFlags::Serialize);
        };
    };
};

#pragma warning(pop)

static_assert(sizeof(FVector) == 16 && alignof(FVector) == 16);

FVector operator*(float Value, const FVector& Vector);
