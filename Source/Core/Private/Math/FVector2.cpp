#include "Core/Math/FVector2.h"
#include "Math/SimdMath.h"
#include "Core/Math/MathUtility.h"

#include <algorithm>
#include <cmath>

using namespace Math::Simd;

FVector2::FVector2(float X, float Y)
	: mValue{_mm_set_ps(0.0f, 0.0f, Y, X)} {
}

FVector2::FVector2(__m128 Value)
	: mValue{_mm_and_ps(Value, _mm_castsi128_ps(_mm_set_epi32(0, 0, -1, -1)))} {
}

FVector2 FVector2::operator+(const FVector2& Other) const {
    return FVector2{_mm_add_ps(mValue, Other.mValue)};
}

FVector2 FVector2::operator-(const FVector2& Other) const {
    return FVector2{_mm_sub_ps(mValue, Other.mValue)};
}

FVector2 FVector2::operator*(const FVector2& Other) const {
    return FVector2{_mm_mul_ps(mValue, Other.mValue)};
}

FVector2 FVector2::operator/(const FVector2& Other) const {
    return FVector2{_mm_div_ps(mValue, Other.mValue)};
}

FVector2 FVector2::operator-() const {
    return FVector2{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
}

FVector2 FVector2::operator*(float Value) const {
    return FVector2{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
}

FVector2 FVector2::operator/(float Value) const {
    return *this * (1.0f / Value);
}

FVector2& FVector2::operator+=(const FVector2& Other) {
    *this = *this + Other;

    return *this;
}

FVector2& FVector2::operator-=(const FVector2& Other) {
    *this = *this - Other;

    return *this;
}

FVector2& FVector2::operator*=(const FVector2& Other) {
    *this = *this * Other;

    return *this;
}

FVector2& FVector2::operator/=(const FVector2& Other) {
    *this = *this / Other;

    return *this;
}

FVector2& FVector2::operator*=(float Value) {
    *this = *this * Value;

    return *this;
}

FVector2& FVector2::operator/=(float Value) {
    *this = *this / Value;

    return *this;
}

bool FVector2::operator==(const FVector2& Other) const {
    return Equal<2>(mValue, Other.mValue);
}

float FVector2::Dot(const FVector2& Other) const {
    return _mm_cvtss_f32(Math::Simd::Dot<2>(mValue, Other.mValue));
}

float FVector2::LengthSquared() const {
    return Dot(*this);
}

float FVector2::Length() const {
    return _mm_cvtss_f32(_mm_sqrt_ps(Math::Simd::Dot<2>(mValue, mValue)));
}

FVector2 FVector2::Normalized() const {
    FVector2 Result{*this};
    Result.Normalize();

    return Result;
}

bool FVector2::Normalize() {
    return Math::Simd::Normalize<2>(mValue);
}

float FVector2::DistanceSquared(const FVector2& Other) const {
    return (*this - Other).LengthSquared();
}

float FVector2::Distance(const FVector2& Other) const {
    return (*this - Other).Length();
}

FVector2 FVector2::Lerp(const FVector2& Other, float Alpha) const {
    return *this + (Other - *this) * Alpha;
}

FVector2 FVector2::Abs() const {
    return FVector2{_mm_andnot_ps(_mm_set1_ps(-0.0f), mValue)};
}

FVector2 FVector2::Min(const FVector2& Other) const {
    return FVector2{_mm_min_ps(mValue, Other.mValue)};
}

FVector2 FVector2::Max(const FVector2& Other) const {
    return FVector2{_mm_max_ps(mValue, Other.mValue)};
}

FVector2 FVector2::Clamped(const FVector2& Minimum, const FVector2& Maximum) const {
    return Max(Minimum).Min(Maximum);
}

FVector2 FVector2::ClampedLength(float MaximumLength) const {
    const float Limit{std::max(MaximumLength, 0.0f)};

    const double SizeSquared{static_cast<double>(x) * x + static_cast<double>(y) * y};

    return SizeSquared > static_cast<double>(Limit) * Limit ? Normalized() * Limit : *this;
}

bool FVector2::IsNearlyZero(float Tolerance) const {
    return NearEqual<2>(mValue, _mm_setzero_ps(), Tolerance);
}

bool FVector2::IsNormalized(float Tolerance) const {
    return IsFinite() && std::abs(LengthSquared() - 1.0f) <= Tolerance;
}

bool FVector2::IsFinite() const {
    return Math::Simd::IsFinite<2>(mValue);
}

bool FVector2::Equals(const FVector2& Other, float Tolerance) const {
    return NearEqual<2>(mValue, Other.mValue, Tolerance);
}

FVector2 operator*(float Value, const FVector2& Vector) {
    return Vector * Value;
}

float FVector2::Cross(const FVector2& Other) const {
    return x * Other.y - y * Other.x;
}

FVector2 FVector2::Rotated(float Degrees) const {
    const float Radians{Math::ToRadians(Degrees)};
    const float Sine{std::sin(Radians)};
    const float Cosine{std::cos(Radians)};

    return {x * Cosine - y * Sine, x * Sine + y * Cosine};
}

std::array<float, 2> FVector2::ToArray() const {
    return {x, y};
}
