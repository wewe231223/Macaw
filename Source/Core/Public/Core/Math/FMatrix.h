#pragma once

#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"

struct FMatrix {
public:
    FMatrix();

public:
    static FMatrix Identity();
    static FMatrix Rotation(const FRotator& Value);
    static FMatrix RotationAxis(const FVector& Axis, float Radians);

    static FMatrix LookTo(const FVector& Position, const FVector& Forward, const FVector& Up);
    static FMatrix Perspective(float FieldOfView, float Aspect, float NearPlane, float FarPlane);

    FMatrix operator*(const FMatrix& Other) const;

    FVector TransformPosition(const FVector& Position) const;
    FVector TransformVector(const FVector& Vector) const;

    FVector GetAxis(Int32 Index) const;
    FRotator GetRotation() const;

    bool TryInverse(FMatrix& Result) const;

public:
    float mValues[4][4]{};
};
