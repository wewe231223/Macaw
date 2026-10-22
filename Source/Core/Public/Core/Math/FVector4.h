#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <xmmintrin.h>

struct FMatrix;
struct FVector;

#pragma warning(push)
#pragma warning(disable: 4201)

struct alignas(16) FVector4 {
public:
    ReflectTypes(FVector4);

public:
    FVector4() = default;
    FVector4(float X, float Y, float Z, float W);
    FVector4(const FVector& Vector, float W);

private:
    explicit FVector4(__m128 Value);

public:
    ~FVector4() = default;
    FVector4(const FVector4& Other) = default;
    FVector4& operator=(const FVector4& Other) = default;
    FVector4(FVector4&& Other) noexcept = default;
    FVector4& operator=(FVector4&& Other) noexcept = default;

public:
    ReflectStruct(FVector4);

    FVector4 operator*(const FMatrix& Matrix) const;
    FVector4& operator*=(const FMatrix& Matrix);

    FVector4 operator+(const FVector4& Other) const;
    FVector4 operator-(const FVector4& Other) const;
    FVector4 operator-() const;
    FVector4 operator*(const FVector4& Other) const;
    FVector4 operator/(const FVector4& Other) const;
    FVector4 operator*(float Value) const;
    FVector4 operator/(float Value) const;
    FVector4& operator+=(const FVector4& Other);
    FVector4& operator-=(const FVector4& Other);
    FVector4& operator*=(const FVector4& Other);
    FVector4& operator/=(const FVector4& Other);
    FVector4& operator*=(float Value);
    FVector4& operator/=(float Value);
    bool operator==(const FVector4& Other) const;

    float Dot(const FVector4& Other) const;
    FVector ToVector3() const;
    bool TryPerspectiveDivide(FVector& Result, float Tolerance = 0.000001f) const;
    std::array<float, 4> ToArray() const;

    float LengthSquared() const;
    float Length() const;
    FVector4 Normalized() const;
    bool Normalize();

    float DistanceSquared(const FVector4& Other) const;
    float Distance(const FVector4& Other) const;
    FVector4 Lerp(const FVector4& Other, float Alpha) const;

    FVector4 Abs() const;
    FVector4 Min(const FVector4& Other) const;
    FVector4 Max(const FVector4& Other) const;
    FVector4 Clamped(const FVector4& Minimum, const FVector4& Maximum) const;
    FVector4 ClampedLength(float MaximumLength) const;

    bool IsNearlyZero(float Tolerance = 0.0001f) const;
    bool IsNormalized(float Tolerance = 0.0001f) const;

    bool IsFinite() const;
    bool Equals(const FVector4& Other, float Tolerance = 0.0001f) const;

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

            float w;
            ReflectProperty(w, "W", EPropertyFlags::Edit | EPropertyFlags::Serialize);
        };
    };
};

#pragma warning(pop)

static_assert(sizeof(FVector4) == 16 && alignof(FVector4) == 16);

FVector4 operator*(float Value, const FVector4& Vector);
