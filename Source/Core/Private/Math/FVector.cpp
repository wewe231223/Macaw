/**
 * @file FVector.cpp
 * @brief 16바이트 정렬 SIMD 저장소를 사용하는 3차원 벡터입니다.
 * @ingroup Core
 */

#include "Core/Math/FVector.h"
#include "Math/SimdMath.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/MathUtility.h"

#include <algorithm>
#include <cmath>

using namespace Math::Simd;

/**
 * @brief FVector의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FVector::FVector(float X, float Y, float Z)
	: mValue{_mm_set_ps(0.0f, Z, Y, X)} {
}

/**
 * @brief FVector의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FVector::FVector(__m128 Value)
	: mValue{_mm_and_ps(Value, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)))} {
}

/**
 * @brief 대응 성분을 더한 값을 반환합니다.
 */
FVector FVector::operator+(const FVector& Other) const {
    return FVector{_mm_add_ps(mValue, Other.mValue)};
}

/**
 * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
 */
FVector FVector::operator-(const FVector& Other) const {
    return FVector{_mm_sub_ps(mValue, Other.mValue)};
}

/**
 * @brief 대응하는 성분끼리 곱합니다.
 */
FVector FVector::operator*(const FVector& Other) const {
    return FVector{_mm_mul_ps(mValue, Other.mValue)};
}

/**
 * @brief 성분별 또는 스칼라 나눗셈 결과를 반환합니다.
 */
FVector FVector::operator/(const FVector& Other) const {
    return FVector{_mm_div_ps(mValue, Other.mValue)};
}

/**
 * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
 */
FVector FVector::operator-() const {
    return FVector{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
}

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 */
FVector FVector::operator*(float Value) const {
    return FVector{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
}

/**
 * @brief 성분별 또는 스칼라 나눗셈 결과를 반환합니다.
 */
FVector FVector::operator/(float Value) const {
    return *this * (1.0f / Value);
}

/**
 * @brief 대응 성분을 현재 값에 더합니다.
 */
FVector& FVector::operator+=(const FVector& Other) {
    *this = *this + Other;

    return *this;
}

/**
 * @brief 대응 성분을 현재 값에서 뺍니다.
 */
FVector& FVector::operator-=(const FVector& Other) {
    *this = *this - Other;

    return *this;
}

/**
 * @brief 대응 성분끼리 곱한 결과를 현재 값에 반영합니다.
 */
FVector& FVector::operator*=(const FVector& Other) {
    *this = *this * Other;

    return *this;
}

/**
 * @brief 나눗셈 결과를 현재 값에 반영합니다.
 */
FVector& FVector::operator/=(const FVector& Other) {
    *this = *this / Other;

    return *this;
}

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 */
FVector& FVector::operator*=(float Value) {
    *this = *this * Value;

    return *this;
}

/**
 * @brief 나눗셈 결과를 현재 값에 반영합니다.
 */
FVector& FVector::operator/=(float Value) {
    *this = *this / Value;

    return *this;
}

/**
 * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
 */
bool FVector::operator==(const FVector& Other) const {
    return Equal<3>(mValue, Other.mValue);
}

/**
 * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
 */
float FVector::Dot(const FVector& Other) const {
    return _mm_cvtss_f32(Math::Simd::Dot<3>(mValue, Other.mValue));
}

/**
 * @brief 두 벡터의 외적을 계산합니다.
 */
FVector FVector::Cross(const FVector& Other) const {
    return FVector{Cross3(mValue, Other.mValue)};
}

/**
 * @brief 성분의 제곱합을 반환합니다.
 */
float FVector::LengthSquared() const {
    return Dot(*this);
}

/**
 * @brief 유클리드 길이를 반환합니다.
 */
float FVector::Length() const {
    return _mm_cvtss_f32(_mm_sqrt_ps(Math::Simd::Dot<3>(mValue, mValue)));
}

/**
 * @brief 정규화한 복사본을 반환합니다.
 */
FVector FVector::Normalized() const {
    FVector Result{*this};
    Result.Normalize();

    return Result;
}

/**
 * @brief 값을 제자리에서 정규화합니다.
 */
bool FVector::Normalize() {
    return Math::Simd::Normalize<3>(mValue);
}

/**
 * @brief 두 값 사이 유클리드 거리의 제곱을 구합니다.
 */
float FVector::DistanceSquared(const FVector& Other) const {
    return (*this - Other).LengthSquared();
}

/**
 * @brief 두 값 사이 유클리드 거리를 구합니다.
 */
float FVector::Distance(const FVector& Other) const {
    return (*this - Other).Length();
}

/**
 * @brief 두 값 사이를 주어진 비율로 선형 보간합니다.
 */
FVector FVector::Lerp(const FVector& Other, float Alpha) const {
    return *this + (Other - *this) * Alpha;
}

/**
 * @brief 각 성분의 절댓값을 반환합니다.
 */
FVector FVector::Abs() const {
    return FVector{_mm_andnot_ps(_mm_set1_ps(-0.0f), mValue)};
}

/**
 * @brief 각 성분에서 두 값 중 작은 값을 선택합니다.
 */
FVector FVector::Min(const FVector& Other) const {
    return FVector{_mm_min_ps(mValue, Other.mValue)};
}

/**
 * @brief 각 성분에서 두 값 중 큰 값을 선택합니다.
 */
FVector FVector::Max(const FVector& Other) const {
    return FVector{_mm_max_ps(mValue, Other.mValue)};
}

/**
 * @brief 각 성분을 지정한 최솟값과 최댓값 사이로 제한합니다.
 */
FVector FVector::Clamped(const FVector& Minimum, const FVector& Maximum) const {
    return Max(Minimum).Min(Maximum);
}

/**
 * @brief 방향을 유지하며 벡터 길이를 지정한 상한으로 제한합니다.
 */
FVector FVector::ClampedLength(float MaximumLength) const {
    const float Limit{std::max(MaximumLength, 0.0f)};

    const double SizeSquared{static_cast<double>(x) * x + static_cast<double>(y) * y + static_cast<double>(z) * z};

    return SizeSquared > static_cast<double>(Limit) * Limit ? Normalized() * Limit : *this;
}

/**
 * @brief 모든 유효 성분이 허용 오차 이내에서 0인지 검사합니다.
 */
bool FVector::IsNearlyZero(float Tolerance) const {
    return NearEqual<3>(mValue, _mm_setzero_ps(), Tolerance);
}

/**
 * @brief 제곱 길이가 허용 오차 이내에서 1인지 검사합니다.
 */
bool FVector::IsNormalized(float Tolerance) const {
    return IsFinite() && std::abs(LengthSquared() - 1.0f) <= Tolerance;
}

/**
 * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
 */
bool FVector::IsFinite() const {
    return Math::Simd::IsFinite<3>(mValue);
}

/**
 * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
 */
bool FVector::Equals(const FVector& Other, float Tolerance) const {
    return NearEqual<3>(mValue, Other.mValue, Tolerance);
}

/**
 * @brief 각 성분에 스칼라 값을 곱합니다.
 */
FVector operator*(float Value, const FVector& Vector) {
    return Vector * Value;
}

/**
 * @brief 유효한 성분을 원래 순서의 일반 배열로 반환합니다.
 */
std::array<float, 3> FVector::ToArray() const {
    return {x, y, z};
}

/**
 * @brief 벡터를 지정한 방향에 정사영합니다.
 */
FVector FVector::ProjectedOn(const FVector& Direction) const {
    const FVector Unit{Direction.Normalized()};

    return Unit * Dot(Unit);
}

/**
 * @brief 벡터를 지정한 법선의 평면에 정사영합니다.
 */
FVector FVector::ProjectedOnPlane(const FVector& Normal) const {
    return *this - ProjectedOn(Normal);
}

/**
 * @brief 지정한 법선을 기준으로 반사한 벡터를 반환합니다.
 * @param[in] Normal 평면 또는 표면 법선입니다.
 * @return 연산을 적용한 새 값입니다.
 */
FVector FVector::Reflected(const FVector& Normal) const {
    return *this - ProjectedOn(Normal) * 2.0f;
}

/**
 * @brief 지정한 축을 중심으로 도 단위 각도만큼 회전합니다.
 */
FVector FVector::RotatedAngleAxis(float Degrees, const FVector& Axis) const {
    return FQuat{Axis, Math::ToRadians(Degrees)} * *this;
}

/**
 * @brief 벡터 방향을 향하는 도 단위 Pitch와 Yaw 회전을 구합니다.
 */
FRotator FVector::Rotation() const {
    return {Math::ToDegrees(std::atan2(z, std::hypot(x, y))), Math::ToDegrees(std::atan2(y, x)), 0.0f};
}
