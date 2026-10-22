#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <xmmintrin.h>

struct FVector;
struct FRotator;

#pragma warning(push)
#pragma warning(disable: 4201)

struct alignas(16) FQuat {
public:
    ReflectTypes(FQuat);

private:
    friend struct FRotator;

public:
    FQuat();
    FQuat(float X, float Y, float Z, float W);
    explicit FQuat(const FRotator& Rotation);
    FQuat(const FVector& Axis, float Radians);

private:
    explicit FQuat(__m128 Value);

public:
    ~FQuat() = default;
    FQuat(const FQuat& Other) = default;
    FQuat& operator=(const FQuat& Other) = default;
    FQuat(FQuat&& Other) noexcept = default;
    FQuat& operator=(FQuat&& Other) noexcept = default;

public:
    ReflectStruct(FQuat);

    FQuat operator*(const FQuat& Other) const;
    FQuat& operator*=(const FQuat& Other);
    FVector operator*(const FVector& Vector) const;
    FQuat operator-() const;
    bool operator==(const FQuat& Other) const;

    float Dot(const FQuat& Other) const;
    float LengthSquared() const;
    float Length() const;
    FQuat Normalized() const;
    bool Normalize();
    FQuat Conjugate() const;
    FQuat Inverse() const;
    FRotator Rotator() const;

    FQuat Nlerp(const FQuat& Other, float Alpha) const;
    FQuat Slerp(const FQuat& Other, float Alpha) const;
    float AngularDistance(const FQuat& Other) const;
    void ToAxisAngle(FVector& Axis, float& Radians) const;

    FVector RotateVector(const FVector& Vector) const;
    FVector UnrotateVector(const FVector& Vector) const;
    FVector ForwardVector() const;
    FVector RightVector() const;
    FVector UpVector() const;

    bool IsFinite() const;
    bool IsNormalized(float Tolerance = 0.0001f) const;
    bool Equals(const FQuat& Other, float Tolerance = 0.0001f) const;

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

static_assert(sizeof(FQuat) == 16 && alignof(FQuat) == 16);
