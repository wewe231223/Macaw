#pragma once

#include "Core/Math/FMatrix.h"

struct FTransform {
public:
    ReflectTypes(FTransform);

public:
    ReflectStruct(FTransform);
    FMatrix ToMatrix() const;
    bool IsValid() const;

private:
    ReflectMembers();

public:
    FVector mLocation{};
    FRotator mRotation{};
    FVector mScale{1.0f, 1.0f, 1.0f};
};
