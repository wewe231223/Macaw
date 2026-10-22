#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FVector.h"
#include "Math/SimdMath.h"

#include <cmath>
#include <numbers>

using namespace Math::Simd;

FRotator::FRotator(float Pitch, float Yaw, float Roll)
	: mValue{_mm_set_ps(0.0f, Roll, Yaw, Pitch)} {
}

FRotator::FRotator(__m128 Value)
	: mValue{_mm_and_ps(Value, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)))} {
}

FRotator::FRotator(const FQuat& Rotation) {
    *this = Rotation.Rotator();
}

FRotator FRotator::operator+(const FRotator& Other) const {
    return FRotator{_mm_add_ps(mValue, Other.mValue)};
}

FRotator FRotator::operator-(const FRotator& Other) const {
    return FRotator{_mm_sub_ps(mValue, Other.mValue)};
}

FRotator FRotator::operator-() const {
    return FRotator{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
}

FRotator FRotator::operator*(float Value) const {
    return FRotator{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
}

FRotator FRotator::operator/(float Value) const {
    return *this * (1.0f / Value);
}

FRotator& FRotator::operator+=(const FRotator& Other) {
    *this = *this + Other;

    return *this;
}

FRotator& FRotator::operator-=(const FRotator& Other) {
    *this = *this - Other;

    return *this;
}

FRotator& FRotator::operator*=(float Value) {
    *this = *this * Value;

    return *this;
}

FRotator& FRotator::operator/=(float Value) {
    *this = *this / Value;

    return *this;
}

bool FRotator::operator==(const FRotator& Other) const {
    return Equal<3>(mValue, Other.mValue);
}

FQuat FRotator::Quaternion() const {
    constexpr float HalfRadiansPerDegree{std::numbers::pi_v<float> / 360.0f};
    const float HalfYaw{yaw * HalfRadiansPerDegree};
    const float HalfPitch{pitch * HalfRadiansPerDegree};
    const float HalfRoll{roll * HalfRadiansPerDegree};
    const __m128 Yaw{_mm_set_ps(std::cos(HalfYaw), std::sin(HalfYaw), 0.0f, 0.0f)};
    const __m128 Pitch{_mm_set_ps(std::cos(HalfPitch), 0.0f, -std::sin(HalfPitch), 0.0f)};
    const __m128 Roll{_mm_set_ps(std::cos(HalfRoll), 0.0f, 0.0f, -std::sin(HalfRoll))};
    const __m128 Rotation{MultiplyQuaternion(MultiplyQuaternion(Yaw, Pitch), Roll)};

    return FQuat{_mm_div_ps(Rotation, _mm_sqrt_ps(Math::Simd::Dot<4>(Rotation, Rotation)))};
}

FVector FRotator::Vector() const {
    return Quaternion().ForwardVector();
}

FVector FRotator::RightVector() const {
    return Quaternion().RightVector();
}

FVector FRotator::UpVector() const {
    return Quaternion().UpVector();
}

FVector FRotator::RotateVector(const FVector& Vector) const {
    return Quaternion().RotateVector(Vector);
}

FVector FRotator::UnrotateVector(const FVector& Vector) const {
    return Quaternion().UnrotateVector(Vector);
}

FRotator FRotator::Inverse() const {
    return Quaternion().Conjugate().Rotator();
}

FRotator FRotator::Normalized() const {
    return {std::remainder(pitch, 360.0f), std::remainder(yaw, 360.0f), std::remainder(roll, 360.0f)};
}

void FRotator::Normalize() {
    *this = Normalized();
}

FRotator FRotator::Delta(const FRotator& Other) const {
    return (Other - *this).Normalized();
}

FRotator FRotator::Lerp(const FRotator& Other, float Alpha) const {
    return (*this + Delta(Other) * Alpha).Normalized();
}

bool FRotator::IsNearlyZero(float Tolerance) const {
    return NearEqual<3>(Normalized().mValue, _mm_setzero_ps(), Tolerance);
}

bool FRotator::Equals(const FRotator& Other, float Tolerance) const {
    return NearEqual<3>(Delta(Other).mValue, _mm_setzero_ps(), Tolerance);
}

bool FRotator::IsFinite() const {
    return Math::Simd::IsFinite<3>(mValue);
}

FRotator operator*(float Value, const FRotator& Rotation) {
    return Rotation * Value;
}

std::array<float, 3> FRotator::ToArray() const {
    return {pitch, yaw, roll};
}
