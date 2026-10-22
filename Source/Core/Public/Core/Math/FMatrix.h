#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"

struct FTransform;
struct FVector4;

struct alignas(16) FMatrix {
public:
    ReflectTypes(FMatrix);

private:
    friend struct FBox;
    friend struct FVector4;

public:
    FMatrix();
    ~FMatrix() = default;
    FMatrix(const FMatrix& Other) = default;
    FMatrix& operator=(const FMatrix& Other) = default;
    FMatrix(FMatrix&& Other) noexcept = default;
    FMatrix& operator=(FMatrix&& Other) noexcept = default;

public:
    ReflectStruct(FMatrix);

    static FMatrix Identity();
    static FMatrix Translation(const FVector& Value);
    static FMatrix Scale(const FVector& Value);
    static FMatrix Rotation(const FRotator& Value);
    static FMatrix Rotation(const FQuat& Value);
    static FMatrix RotationAxis(const FVector& Axis, float Radians);
    static FMatrix FromTransform(const FTransform& Transform);

    static FMatrix LookTo(const FVector& Position, const FVector& Forward, const FVector& Up);
    static FMatrix Perspective(float FieldOfView, float Aspect, float NearPlane, float FarPlane);

    FMatrix operator+(const FMatrix& Other) const;
    FMatrix operator-(const FMatrix& Other) const;
    FMatrix operator*(const FMatrix& Other) const;
    FMatrix operator*(float Value) const;
    FMatrix operator/(float Value) const;
    FMatrix& operator+=(const FMatrix& Other);
    FMatrix& operator-=(const FMatrix& Other);
    FMatrix& operator*=(const FMatrix& Other);
    FMatrix& operator*=(float Value);
    FMatrix& operator/=(float Value);
    bool operator==(const FMatrix& Other) const;

    FVector TransformPosition(const FVector& Position) const;
    FVector TransformVector(const FVector& Vector) const;

    FVector GetAxis(Int32 Index) const;
    FRotator GetRotation() const;
    FQuat GetQuaternion() const;

    bool TryInverse(FMatrix& Result) const;
    FMatrix Transposed() const;
    float Determinant() const;
    FVector Origin() const;

    bool IsFinite() const;
    bool IsIdentity(float Tolerance = 0.0001f) const;
    bool Equals(const FMatrix& Other, float Tolerance = 0.0001f) const;

    float& operator()(Int32 Row, Int32 Column);
    const float& operator()(Int32 Row, Int32 Column) const;

private:
    ReflectMembers();

private:
    __m128 mRows[4]{};
    ReflectProperty(mRows, "Rows", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

FMatrix operator*(float Value, const FMatrix& Matrix);
