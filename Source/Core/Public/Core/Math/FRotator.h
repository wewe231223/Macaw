#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <xmmintrin.h>

struct FQuat;
struct FVector;

#pragma warning(push)
#pragma warning(disable: 4201)

struct alignas(16) FRotator {
public:
    ReflectTypes(FRotator);

public:
    FRotator() = default;
    FRotator(float Pitch, float Yaw, float Roll);
    explicit FRotator(const FQuat& Rotation);

private:
    explicit FRotator(__m128 Value);

public:
    ~FRotator() = default;
    FRotator(const FRotator& Other) = default;
    FRotator& operator=(const FRotator& Other) = default;
    FRotator(FRotator&& Other) noexcept = default;
    FRotator& operator=(FRotator&& Other) noexcept = default;

public:
    ReflectStruct(FRotator);

    FRotator operator+(const FRotator& Other) const;
    FRotator operator-(const FRotator& Other) const;
    FRotator operator-() const;
    FRotator operator*(float Value) const;
    FRotator operator/(float Value) const;
    FRotator& operator+=(const FRotator& Other);
    FRotator& operator-=(const FRotator& Other);
    FRotator& operator*=(float Value);
    FRotator& operator/=(float Value);
    bool operator==(const FRotator& Other) const;

    FQuat Quaternion() const;
    FVector Vector() const;
    FVector RightVector() const;
    FVector UpVector() const;
    FVector RotateVector(const FVector& Vector) const;
    FVector UnrotateVector(const FVector& Vector) const;
    FRotator Inverse() const;
    FRotator Normalized() const;
    void Normalize();
    FRotator Delta(const FRotator& Other) const;
    FRotator Lerp(const FRotator& Other, float Alpha) const;
    bool IsNearlyZero(float Tolerance = 0.0001f) const;
    bool Equals(const FRotator& Other, float Tolerance = 0.0001f) const;
    bool IsFinite() const;

    std::array<float, 3> ToArray() const;

private:
    ReflectMembers();

public:
    union {
        __m128 mValue{};
        ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

        struct {
            float pitch;
            ReflectProperty(pitch, "Pitch", EPropertyFlags::Edit | EPropertyFlags::Serialize);


            float yaw;
            ReflectProperty(yaw, "Yaw", EPropertyFlags::Edit | EPropertyFlags::Serialize);


            float roll;
            ReflectProperty(roll, "Roll", EPropertyFlags::Edit | EPropertyFlags::Serialize);
        };
    };
};

#pragma warning(pop)

static_assert(sizeof(FRotator) == 16 && alignof(FRotator) == 16);

FRotator operator*(float Value, const FRotator& Rotation);
