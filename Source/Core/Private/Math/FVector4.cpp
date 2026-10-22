#include "Core/Math/FVector4.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Math/SimdMath.h"

#include <algorithm>
#include <cmath>

using namespace Math::Simd;

FVector4::FVector4(float X, float Y, float Z, float W)
	: mValue{_mm_set_ps(W, Z, Y, X)} {
}

FVector4::FVector4(const FVector& Vector, float W)
	: FVector4{Vector.x, Vector.y, Vector.z, W} {
}

FVector4::FVector4(__m128 Value)
	: mValue{Value} {
}

FVector4 FVector4::operator*(const FMatrix& Matrix) const {
    return FVector4{Math::Simd::Transform(mValue, Matrix.mRows)};
}

FVector4& FVector4::operator*=(const FMatrix& Matrix) {
    *this = *this * Matrix;

    return *this;
}

FVector4 FVector4::operator+(const FVector4& Other) const {
    return FVector4{_mm_add_ps(mValue, Other.mValue)};
}

FVector4 FVector4::operator-(const FVector4& Other) const {
    return FVector4{_mm_sub_ps(mValue, Other.mValue)};
}

FVector4 FVector4::operator*(const FVector4& Other) const {
    return FVector4{_mm_mul_ps(mValue, Other.mValue)};
}

FVector4 FVector4::operator/(const FVector4& Other) const {
    return FVector4{_mm_div_ps(mValue, Other.mValue)};
}

FVector4 FVector4::operator-() const {
    return FVector4{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
}

FVector4 FVector4::operator*(float Value) const {
    return FVector4{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
}

FVector4 FVector4::operator/(float Value) const {
    return *this * (1.0f / Value);
}

FVector4& FVector4::operator+=(const FVector4& Other) {
    *this = *this + Other;

    return *this;
}

FVector4& FVector4::operator-=(const FVector4& Other) {
    *this = *this - Other;

    return *this;
}

FVector4& FVector4::operator*=(const FVector4& Other) {
    *this = *this * Other;

    return *this;
}

FVector4& FVector4::operator/=(const FVector4& Other) {
    *this = *this / Other;

    return *this;
}

FVector4& FVector4::operator*=(float Value) {
    *this = *this * Value;

    return *this;
}

FVector4& FVector4::operator/=(float Value) {
    *this = *this / Value;

    return *this;
}

bool FVector4::operator==(const FVector4& Other) const {
    return Equal<4>(mValue, Other.mValue);
}

float FVector4::Dot(const FVector4& Other) const {
    return _mm_cvtss_f32(Math::Simd::Dot<4>(mValue, Other.mValue));
}

float FVector4::LengthSquared() const {
    return Dot(*this);
}

float FVector4::Length() const {
    return _mm_cvtss_f32(_mm_sqrt_ps(Math::Simd::Dot<4>(mValue, mValue)));
}

FVector4 FVector4::Normalized() const {
    FVector4 Result{*this};
    Result.Normalize();

    return Result;
}

bool FVector4::Normalize() {
    return Math::Simd::Normalize<4>(mValue);
}

float FVector4::DistanceSquared(const FVector4& Other) const {
    return (*this - Other).LengthSquared();
}

float FVector4::Distance(const FVector4& Other) const {
    return (*this - Other).Length();
}

FVector4 FVector4::Lerp(const FVector4& Other, float Alpha) const {
    return *this + (Other - *this) * Alpha;
}

FVector4 FVector4::Abs() const {
    return FVector4{_mm_andnot_ps(_mm_set1_ps(-0.0f), mValue)};
}

FVector4 FVector4::Min(const FVector4& Other) const {
    return FVector4{_mm_min_ps(mValue, Other.mValue)};
}

FVector4 FVector4::Max(const FVector4& Other) const {
    return FVector4{_mm_max_ps(mValue, Other.mValue)};
}

FVector4 FVector4::Clamped(const FVector4& Minimum, const FVector4& Maximum) const {
    return Max(Minimum).Min(Maximum);
}

FVector4 FVector4::ClampedLength(float MaximumLength) const {
    const float Limit{std::max(MaximumLength, 0.0f)};

    const double SizeSquared{static_cast<double>(x) * x + static_cast<double>(y) * y + static_cast<double>(z) * z + static_cast<double>(w) * w};

    return SizeSquared > static_cast<double>(Limit) * Limit ? Normalized() * Limit : *this;
}

bool FVector4::IsNearlyZero(float Tolerance) const {
    return NearEqual<4>(mValue, _mm_setzero_ps(), Tolerance);
}

bool FVector4::IsNormalized(float Tolerance) const {
    return IsFinite() && std::abs(LengthSquared() - 1.0f) <= Tolerance;
}

bool FVector4::IsFinite() const {
    return Math::Simd::IsFinite<4>(mValue);
}

bool FVector4::Equals(const FVector4& Other, float Tolerance) const {
    return NearEqual<4>(mValue, Other.mValue, Tolerance);
}

FVector4 operator*(float Value, const FVector4& Vector) {
    return Vector * Value;
}

FVector FVector4::ToVector3() const {
    return {x, y, z};
}

bool FVector4::TryPerspectiveDivide(FVector& Result, float Tolerance) const {
    if (!IsFinite() || std::abs(w) <= std::abs(Tolerance)) {
        return false;
    }

    const FVector Candidate{x / w, y / w, z / w};

    if (!Candidate.IsFinite()) {
        return false;
    }

    Result = Candidate;

    return true;
}

std::array<float, 4> FVector4::ToArray() const {
    return {x, y, z, w};
}
