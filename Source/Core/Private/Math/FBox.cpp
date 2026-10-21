#include "Core/Math/FBox.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRay.h"

#include <algorithm>
#include <cmath>
#include <limits>

bool FBox::Intersects(const FRay& Ray, float MaximumDistance) const {
    float MinimumDistance{};
    float Maximum{MaximumDistance};

    const std::array<float, 3> Origins{Ray.mOrigin.mX, Ray.mOrigin.mY, Ray.mOrigin.mZ};
    const std::array<float, 3> Directions{Ray.mDirection.mX, Ray.mDirection.mY, Ray.mDirection.mZ};

    const std::array<float, 3> Minimums{mMinimum.mX, mMinimum.mY, mMinimum.mZ};
    const std::array<float, 3> Maximums{mMaximum.mX, mMaximum.mY, mMaximum.mZ};

    for (Int32 Index{}; Index < 3; ++Index) {
        if (std::abs(Directions[Index]) < 0.0000001f) {
            if (Origins[Index] < Minimums[Index] || Origins[Index] > Maximums[Index]) {
                return false;
            }

            continue;
        }

        float Near{(Minimums[Index] - Origins[Index]) / Directions[Index]};
        float Far{(Maximums[Index] - Origins[Index]) / Directions[Index]};

        if (Near > Far) {
            std::swap(Near, Far);
        }

        MinimumDistance = std::max(MinimumDistance, Near);
        Maximum = std::min(Maximum, Far);

        if (MinimumDistance > Maximum) {
            return false;
        }
    }

    return true;
}

FBox FBox::Transformed(const FMatrix& Matrix) const {
    const float Limit{std::numeric_limits<float>::max()};
    FBox Result{{Limit, Limit, Limit}, {-Limit, -Limit, -Limit}};

    for (Int32 Index{}; Index < 8; ++Index) {
        const FVector Point{Matrix.TransformPosition({(Index & 1) ? mMaximum.mX : mMinimum.mX, (Index & 2) ? mMaximum.mY : mMinimum.mY, (Index & 4) ? mMaximum.mZ : mMinimum.mZ})};

        Result.mMinimum = {std::min(Result.mMinimum.mX, Point.mX), std::min(Result.mMinimum.mY, Point.mY), std::min(Result.mMinimum.mZ, Point.mZ)};
        Result.mMaximum = {std::max(Result.mMaximum.mX, Point.mX), std::max(Result.mMaximum.mY, Point.mY), std::max(Result.mMaximum.mZ, Point.mZ)};
    }

    return Result;
}
