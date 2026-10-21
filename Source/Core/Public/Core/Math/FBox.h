#pragma once

#include "Core/Math/FVector.h"

struct FRay;
struct FMatrix;

struct FBox {
public:
    bool Intersects(const FRay& Ray, float MaximumDistance) const;
    FBox Transformed(const FMatrix& Matrix) const;

public:
    FVector mMinimum{};
    FVector mMaximum{};
};
