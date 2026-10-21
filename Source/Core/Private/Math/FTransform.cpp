#include "Core/Math/FTransform.h"
#include "Core/Math/FMatrix.h"

#include <cmath>

FMatrix FTransform::ToMatrix() const {
    FMatrix Scale{};
    Scale.mValues[0][0] = mScale.mX;
    Scale.mValues[1][1] = mScale.mY;
    Scale.mValues[2][2] = mScale.mZ;

    FMatrix Translation{};
    Translation.mValues[3][0] = mLocation.mX;
    Translation.mValues[3][1] = mLocation.mY;
    Translation.mValues[3][2] = mLocation.mZ;

    return Scale * FMatrix::Rotation(mRotation) * Translation;
}

bool FTransform::IsValid() const {
    return mLocation.IsFinite() && mScale.IsFinite() && std::isfinite(mRotation.mPitch) && std::isfinite(mRotation.mYaw) && std::isfinite(mRotation.mRoll) && std::abs(mScale.mX) >= 0.0001f && std::abs(mScale.mY) >= 0.0001f && std::abs(mScale.mZ) >= 0.0001f;
}
