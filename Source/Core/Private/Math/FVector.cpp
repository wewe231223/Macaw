#include "Core/Math/FVector.h"
#include "Math/SimdMath.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/MathUtility.h"

#include <algorithm>
#include <cmath>

using namespace Math::Simd;

FVector::FVector(float X, float Y, float Z)
	: mValue{_mm_set_ps(0.0f, Z, Y, X)} {
}

FVector::FVector(__m128 Value)
	: mValue{_mm_and_ps(Value, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)))} {
}

FVector FVector::operator+(const FVector& Other) const {
    return FVector{_mm_add_ps(mValue, Other.mValue)};
}

FVector FVector::operator-(const FVector& Other) const {
    return FVector{_mm_sub_ps(mValue, Other.mValue)};
}

FVector FVector::operator*(const FVector& Other) const {
    return FVector{_mm_mul_ps(mValue, Other.mValue)};
}

FVector FVector::operator/(const FVector& Other) const {
    return FVector{_mm_div_ps(mValue, Other.mValue)};
}

FVector FVector::operator-() const {
    return FVector{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
}

FVector FVector::operator*(float Value) const {
    return FVector{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
}

FVector FVector::operator/(float Value) const {
    return *this * (1.0f / Value);
}

FVector& FVector::operator+=(const FVector& Other) {
    *this = *this + Other;

    return *this;
}

FVector& FVector::operator-=(const FVector& Other) {
    *this = *this - Other;

    return *this;
}

FVector& FVector::operator*=(const FVector& Other) {
    *this = *this * Other;

    return *this;
}

FVector& FVector::operator/=(const FVector& Other) {
    *this = *this / Other;

    return *this;
}

FVector& FVector::operator*=(float Value) {
    *this = *this * Value;

    return *this;
}

FVector& FVector::operator/=(float Value) {
    *this = *this / Value;

    return *this;
}

bool FVector::operator==(const FVector& Other) const {
    return Equal<3>(mValue, Other.mValue);
}

float FVector::Dot(const FVector& Other) const {
    return _mm_cvtss_f32(Math::Simd::Dot<3>(mValue, Other.mValue));
}

FVector FVector::Cross(const FVector& Other) const {
    return FVector{Cross3(mValue, Other.mValue)};
}

float FVector::LengthSquared() const {
    return Dot(*this);
}

float FVector::Length() const {
    return _mm_cvtss_f32(_mm_sqrt_ps(Math::Simd::Dot<3>(mValue, mValue)));
}

FVector FVector::Normalized() const {
    FVector Result{*this};
    Result.Normalize();

    return Result;
}

bool FVector::Normalize() {
    return Math::Simd::Normalize<3>(mValue);
}

float FVector::DistanceSquared(const FVector& Other) const {
    return (*this - Other).LengthSquared();
}

float FVector::Distance(const FVector& Other) const {
    return (*this - Other).Length();
}

FVector FVector::Lerp(const FVector& Other, float Alpha) const {
    return *this + (Other - *this) * Alpha;
}

FVector FVector::Abs() const {
    return FVector{_mm_andnot_ps(_mm_set1_ps(-0.0f), mValue)};
}

FVector FVector::Min(const FVector& Other) const {
    return FVector{_mm_min_ps(mValue, Other.mValue)};
}

FVector FVector::Max(const FVector& Other) const {
    return FVector{_mm_max_ps(mValue, Other.mValue)};
}

FVector FVector::Clamped(const FVector& Minimum, const FVector& Maximum) const {
    return Max(Minimum).Min(Maximum);
}

FVector FVector::ClampedLength(float MaximumLength) const {
    const float Limit{std::max(MaximumLength, 0.0f)};

    const double SizeSquared{static_cast<double>(x) * x + static_cast<double>(y) * y + static_cast<double>(z) * z};

    return SizeSquared > static_cast<double>(Limit) * Limit ? Normalized() * Limit : *this;
}

bool FVector::IsNearlyZero(float Tolerance) const {
    return NearEqual<3>(mValue, _mm_setzero_ps(), Tolerance);
}

bool FVector::IsNormalized(float Tolerance) const {
    return IsFinite() && std::abs(LengthSquared() - 1.0f) <= Tolerance;
}

bool FVector::IsFinite() const {
    return Math::Simd::IsFinite<3>(mValue);
}

bool FVector::Equals(const FVector& Other, float Tolerance) const {
    return NearEqual<3>(mValue, Other.mValue, Tolerance);
}

FVector operator*(float Value, const FVector& Vector) {
    return Vector * Value;
}

std::array<float, 3> FVector::ToArray() const {
    return {x, y, z};
}

FVector FVector::ProjectedOn(const FVector& Direction) const {
    const FVector Unit{Direction.Normalized()};

    return Unit * Dot(Unit);
}

FVector FVector::ProjectedOnPlane(const FVector& Normal) const {
    return *this - ProjectedOn(Normal);
}

FVector FVector::Reflected(const FVector& Normal) const {
    return *this - ProjectedOn(Normal) * 2.0f;
}

FVector FVector::RotatedAngleAxis(float Degrees, const FVector& Axis) const {
    return FQuat{Axis, Math::ToRadians(Degrees)} * *this;
}

FRotator FVector::Rotation() const {
    return {Math::ToDegrees(std::atan2(z, std::hypot(x, y))), Math::ToDegrees(std::atan2(y, x)), 0.0f};
}
