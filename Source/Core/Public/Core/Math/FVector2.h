#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <xmmintrin.h>

#pragma warning(push)
#pragma warning(disable: 4201)

struct alignas(16) FVector2 {
public:
    ReflectTypes(FVector2);

public:
    FVector2() = default;
    FVector2(float X, float Y);

private:
    explicit FVector2(__m128 Value);

public:
    ~FVector2() = default;
    FVector2(const FVector2& Other) = default;
    FVector2& operator=(const FVector2& Other) = default;
    FVector2(FVector2&& Other) noexcept = default;
    FVector2& operator=(FVector2&& Other) noexcept = default;

public:
    ReflectStruct(FVector2);

    FVector2 operator+(const FVector2& Other) const;
    FVector2 operator-(const FVector2& Other) const;
    FVector2 operator-() const;
    FVector2 operator*(const FVector2& Other) const;
    FVector2 operator/(const FVector2& Other) const;
    FVector2 operator*(float Value) const;
    FVector2 operator/(float Value) const;
    FVector2& operator+=(const FVector2& Other);
    FVector2& operator-=(const FVector2& Other);
    FVector2& operator*=(const FVector2& Other);
    FVector2& operator/=(const FVector2& Other);
    FVector2& operator*=(float Value);
    FVector2& operator/=(float Value);
    bool operator==(const FVector2& Other) const;

    float Dot(const FVector2& Other) const;
    float Cross(const FVector2& Other) const;
    FVector2 Rotated(float Degrees) const;
    std::array<float, 2> ToArray() const;

    float LengthSquared() const;
    float Length() const;
    FVector2 Normalized() const;
    bool Normalize();

    float DistanceSquared(const FVector2& Other) const;
    float Distance(const FVector2& Other) const;
    FVector2 Lerp(const FVector2& Other, float Alpha) const;

    FVector2 Abs() const;
    FVector2 Min(const FVector2& Other) const;
    FVector2 Max(const FVector2& Other) const;
    FVector2 Clamped(const FVector2& Minimum, const FVector2& Maximum) const;
    FVector2 ClampedLength(float MaximumLength) const;

    bool IsNearlyZero(float Tolerance = 0.0001f) const;
    bool IsNormalized(float Tolerance = 0.0001f) const;

    bool IsFinite() const;
    bool Equals(const FVector2& Other, float Tolerance = 0.0001f) const;

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
        };
    };
};

#pragma warning(pop)

static_assert(sizeof(FVector2) == 16 && alignof(FVector2) == 16);

FVector2 operator*(float Value, const FVector2& Vector);
