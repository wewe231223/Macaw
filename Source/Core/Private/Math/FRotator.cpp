/**
 * @file FRotator.cpp
 * @brief 도 단위의 Pitch, Yaw, Roll로 표현하는 오일러 회전입니다.
 * @ingroup Core
 */

#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FVector.h"
#include "Math/SimdMath.h"

#include <cmath>
#include <numbers>

using namespace Math::Simd;

/**
 * @brief FRotator의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FRotator::FRotator(float Pitch, float Yaw, float Roll)
	: mValue{_mm_set_ps(0.0f, Roll, Yaw, Pitch)} {
}

/**
 * @brief FRotator의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FRotator::FRotator(__m128 Value)
	: mValue{_mm_and_ps(Value, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)))} {
}

/**
 * @brief FRotator의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FRotator::FRotator(const FQuat& Rotation) {
    *this = Rotation.Rotator();
}

/**
 * @brief 대응 성분을 더한 값을 반환합니다.
 */
FRotator FRotator::operator+(const FRotator& Other) const {
    return FRotator{_mm_add_ps(mValue, Other.mValue)};
}

/**
 * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
 */
FRotator FRotator::operator-(const FRotator& Other) const {
    return FRotator{_mm_sub_ps(mValue, Other.mValue)};
}

/**
 * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
 */
FRotator FRotator::operator-() const {
    return FRotator{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
}

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 */
FRotator FRotator::operator*(float Value) const {
    return FRotator{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
}

/**
 * @brief 성분별 또는 스칼라 나눗셈 결과를 반환합니다.
 */
FRotator FRotator::operator/(float Value) const {
    return *this * (1.0f / Value);
}

/**
 * @brief 대응 성분을 현재 값에 더합니다.
 */
FRotator& FRotator::operator+=(const FRotator& Other) {
    *this = *this + Other;

    return *this;
}

/**
 * @brief 대응 성분을 현재 값에서 뺍니다.
 */
FRotator& FRotator::operator-=(const FRotator& Other) {
    *this = *this - Other;

    return *this;
}

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 */
FRotator& FRotator::operator*=(float Value) {
    *this = *this * Value;

    return *this;
}

/**
 * @brief 나눗셈 결과를 현재 값에 반영합니다.
 */
FRotator& FRotator::operator/=(float Value) {
    *this = *this / Value;

    return *this;
}

/**
 * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
 */
bool FRotator::operator==(const FRotator& Other) const {
    return Equal<3>(mValue, Other.mValue);
}

/**
 * @brief 도 단위 오일러 각을 쿼터니언 회전으로 변환합니다.
 */
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

/**
 * @brief 오일러 회전의 전방 단위 벡터를 반환합니다.
 */
FVector FRotator::Vector() const {
    return Quaternion().ForwardVector();
}

/**
 * @brief 회전된 우측 단위 벡터를 반환합니다.
 */
FVector FRotator::RightVector() const {
    return Quaternion().RightVector();
}

/**
 * @brief 회전된 상방 단위 벡터를 반환합니다.
 */
FVector FRotator::UpVector() const {
    return Quaternion().UpVector();
}

/**
 * @brief 현재 회전을 벡터에 적용합니다.
 */
FVector FRotator::RotateVector(const FVector& Vector) const {
    return Quaternion().RotateVector(Vector);
}

/**
 * @brief 현재 회전의 역회전을 벡터에 적용합니다.
 */
FVector FRotator::UnrotateVector(const FVector& Vector) const {
    return Quaternion().UnrotateVector(Vector);
}

/**
 * @brief 현재 회전의 역회전을 반환합니다.
 */
FRotator FRotator::Inverse() const {
    return Quaternion().Conjugate().Rotator();
}

/**
 * @brief 각 오일러 각도를 360도 나머지 범위로 정규화한 복사본을 반환합니다.
 */
FRotator FRotator::Normalized() const {
    return {std::remainder(pitch, 360.0f), std::remainder(yaw, 360.0f), std::remainder(roll, 360.0f)};
}

/**
 * @brief 각 오일러 각도를 360도 나머지 범위로 정규화합니다.
 */
void FRotator::Normalize() {
    *this = Normalized();
}

/**
 * @brief 상대 회전과의 오일러 각 차이를 정규화하여 반환합니다.
 */
FRotator FRotator::Delta(const FRotator& Other) const {
    return (Other - *this).Normalized();
}

/**
 * @brief 두 값 사이를 주어진 비율로 선형 보간합니다.
 */
FRotator FRotator::Lerp(const FRotator& Other, float Alpha) const {
    return (*this + Delta(Other) * Alpha).Normalized();
}

/**
 * @brief 모든 유효 성분이 허용 오차 이내에서 0인지 검사합니다.
 */
bool FRotator::IsNearlyZero(float Tolerance) const {
    return NearEqual<3>(Normalized().mValue, _mm_setzero_ps(), Tolerance);
}

/**
 * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
 */
bool FRotator::Equals(const FRotator& Other, float Tolerance) const {
    return NearEqual<3>(Delta(Other).mValue, _mm_setzero_ps(), Tolerance);
}

/**
 * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
 */
bool FRotator::IsFinite() const {
    return Math::Simd::IsFinite<3>(mValue);
}

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 */
FRotator operator*(float Value, const FRotator& Rotation) {
    return Rotation * Value;
}

/**
 * @brief 유효한 성분을 원래 순서의 일반 배열로 반환합니다.
 */
std::array<float, 3> FRotator::ToArray() const {
    return {pitch, yaw, roll};
}
