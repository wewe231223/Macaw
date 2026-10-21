#include "Core/Math/FVector.h"

#include <cmath>

FVector FVector::operator+(const FVector& Other) const {
    return {mX + Other.mX, mY + Other.mY, mZ + Other.mZ};
}

FVector FVector::operator-(const FVector& Other) const {
    return {mX - Other.mX, mY - Other.mY, mZ - Other.mZ};
}

FVector FVector::operator-() const {
    return {-mX, -mY, -mZ};
}

FVector FVector::operator*(float Value) const {
    return {mX * Value, mY * Value, mZ * Value};
}

FVector FVector::operator/(float Value) const {
    return {mX / Value, mY / Value, mZ / Value};
}

FVector& FVector::operator+=(const FVector& Other) {
    mX += Other.mX;
    mY += Other.mY;
    mZ += Other.mZ;

    return *this;
}

float FVector::Dot(const FVector& Other) const {
    return mX * Other.mX + mY * Other.mY + mZ * Other.mZ;
}

FVector FVector::Cross(const FVector& Other) const {
    return {mY * Other.mZ - mZ * Other.mY, mZ * Other.mX - mX * Other.mZ, mX * Other.mY - mY * Other.mX};
}

float FVector::Length() const {
    return std::sqrt(Dot(*this));
}

FVector FVector::Normalized() const {
    const float Size{Length()};
    return Size > 0.000001f ? *this / Size : FVector{};
}

bool FVector::IsFinite() const {
    return std::isfinite(mX) && std::isfinite(mY) && std::isfinite(mZ);
}
