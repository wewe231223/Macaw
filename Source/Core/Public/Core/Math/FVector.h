#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

struct FVector {
public:
    ReflectTypes(FVector);

public:
    ReflectStruct(FVector);

    FVector operator+(const FVector& Other) const;
    FVector operator-(const FVector& Other) const;
    FVector operator-() const;
    FVector operator*(float Value) const;
    FVector operator/(float Value) const;
    FVector& operator+=(const FVector& Other);

    float Dot(const FVector& Other) const;
    FVector Cross(const FVector& Other) const;

    float Length() const;
    FVector Normalized() const;

    bool IsFinite() const;

private:
    ReflectMembers();

public:
    float mX{};
    float mY{};
    float mZ{};
};
