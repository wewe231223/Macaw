/**
 * @file FVector2.cpp
 * @brief 16바이트 정렬 SIMD 저장소를 사용하는 2차원 벡터입니다.
 * @ingroup Core
 */

#include "Core/Math/FVector2.h"
#include "Math/SimdMath.h"
#include "Core/Math/MathUtility.h"

#include <algorithm>
#include <cmath>

namespace Core {

    /**
     * @brief FVector2의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FVector2::FVector2(float X, float Y)
		: mValue{_mm_set_ps(0.0f, 0.0f, Y, X)} {
    }

    /**
     * @brief FVector2의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FVector2::FVector2(__m128 Value)
		: mValue{_mm_and_ps(Value, _mm_castsi128_ps(_mm_set_epi32(0, 0, -1, -1)))} {
    }

    /**
     * @brief 대응 성분을 더한 값을 반환합니다.
     */
    Core::FVector2 Core::FVector2::operator+(const Core::FVector2& Other) const {
        return Core::FVector2{_mm_add_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 대응 성분끼리 뺀 값을 반환합니다.
     */
    Core::FVector2 Core::FVector2::operator-(const Core::FVector2& Other) const {
        return Core::FVector2{_mm_sub_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 대응하는 성분끼리 곱합니다.
     */
    Core::FVector2 Core::FVector2::operator*(const Core::FVector2& Other) const {
        return Core::FVector2{_mm_mul_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 대응 성분끼리 나눈 결과를 반환합니다.
     */
    Core::FVector2 Core::FVector2::operator/(const Core::FVector2& Other) const {
        return Core::FVector2{_mm_div_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 모든 성분의 부호를 반전한 값을 반환합니다.
     */
    Core::FVector2 Core::FVector2::operator-() const {
        return Core::FVector2{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FVector2 Core::FVector2::operator*(float Value) const {
        return Core::FVector2{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
    }

    /**
     * @brief 각 성분을 스칼라 값으로 나눈 결과를 반환합니다.
     */
    Core::FVector2 Core::FVector2::operator/(float Value) const {
        return *this * (1.0f / Value);
    }

    /**
     * @brief 대응 성분을 현재 값에 더합니다.
     */
    Core::FVector2& Core::FVector2::operator+=(const Core::FVector2& Other) {
        *this = *this + Other;

        return *this;
    }

    /**
     * @brief 대응 성분을 현재 값에서 뺍니다.
     */
    Core::FVector2& Core::FVector2::operator-=(const Core::FVector2& Other) {
        *this = *this - Other;

        return *this;
    }

    /**
     * @brief 대응 성분끼리 곱한 결과를 현재 값에 반영합니다.
     */
    Core::FVector2& Core::FVector2::operator*=(const Core::FVector2& Other) {
        *this = *this * Other;

        return *this;
    }

    /**
     * @brief 대응 성분끼리 나누어 현재 값에 반영합니다.
     */
    Core::FVector2& Core::FVector2::operator/=(const Core::FVector2& Other) {
        *this = *this / Other;

        return *this;
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FVector2& Core::FVector2::operator*=(float Value) {
        *this = *this * Value;

        return *this;
    }

    /**
     * @brief 각 성분을 스칼라 값으로 나누어 현재 값에 반영합니다.
     */
    Core::FVector2& Core::FVector2::operator/=(float Value) {
        *this = *this / Value;

        return *this;
    }

    /**
     * @brief 모든 성분이 허용 오차 없이 정확히 같은지 검사합니다.
     */
    bool Core::FVector2::operator==(const Core::FVector2& Other) const {
        return Core::Math::Simd::Equal<2>(mValue, Other.mValue);
    }

    /**
     * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
     */
    float Core::FVector2::Dot(const Core::FVector2& Other) const {
        return _mm_cvtss_f32(Core::Math::Simd::Dot<2>(mValue, Other.mValue));
    }

    /**
     * @brief 성분의 제곱합을 반환합니다.
     */
    float Core::FVector2::LengthSquared() const {
        return Dot(*this);
    }

    /**
     * @brief 유클리드 길이를 반환합니다.
     */
    float Core::FVector2::Length() const {
        return _mm_cvtss_f32(_mm_sqrt_ps(Core::Math::Simd::Dot<2>(mValue, mValue)));
    }

    /**
     * @brief 정규화한 복사본을 반환합니다.
     */
    Core::FVector2 Core::FVector2::Normalized() const {
        Core::FVector2 Result{*this};
        Result.Normalize();

        return Result;
    }

    /**
     * @brief 값을 제자리에서 정규화합니다.
     */
    bool Core::FVector2::Normalize() {
        return Core::Math::Simd::Normalize<2>(mValue);
    }

    /**
     * @brief 두 값 사이 유클리드 거리의 제곱을 구합니다.
     */
    float Core::FVector2::DistanceSquared(const Core::FVector2& Other) const {
        return (*this - Other).LengthSquared();
    }

    /**
     * @brief 두 값 사이 유클리드 거리를 구합니다.
     */
    float Core::FVector2::Distance(const Core::FVector2& Other) const {
        return (*this - Other).Length();
    }

    /**
     * @brief 두 값 사이를 주어진 비율로 선형 보간합니다.
     */
    Core::FVector2 Core::FVector2::Lerp(const Core::FVector2& Other, float Alpha) const {
        return *this + (Other - *this) * Alpha;
    }

    /**
     * @brief 각 성분의 절댓값을 반환합니다.
     */
    Core::FVector2 Core::FVector2::Abs() const {
        return Core::FVector2{_mm_andnot_ps(_mm_set1_ps(-0.0f), mValue)};
    }

    /**
     * @brief 각 성분에서 두 값 중 작은 값을 선택합니다.
     */
    Core::FVector2 Core::FVector2::Min(const Core::FVector2& Other) const {
        return Core::FVector2{_mm_min_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 각 성분에서 두 값 중 큰 값을 선택합니다.
     */
    Core::FVector2 Core::FVector2::Max(const Core::FVector2& Other) const {
        return Core::FVector2{_mm_max_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 각 성분을 지정한 최솟값과 최댓값 사이로 제한합니다.
     */
    Core::FVector2 Core::FVector2::Clamped(const Core::FVector2& Minimum, const Core::FVector2& Maximum) const {
        return Max(Minimum).Min(Maximum);
    }

    /**
     * @brief 방향을 유지하며 벡터 길이를 지정한 상한으로 제한합니다.
     */
    Core::FVector2 Core::FVector2::ClampedLength(float MaximumLength) const {
        const float Limit{std::max(MaximumLength, 0.0f)};

        const double SizeSquared{static_cast<double>(x) * x + static_cast<double>(y) * y};

        return SizeSquared > static_cast<double>(Limit) * Limit ? Normalized() * Limit : *this;
    }

    /**
     * @brief 모든 유효 성분이 허용 오차 이내에서 0인지 검사합니다.
     */
    bool Core::FVector2::IsNearlyZero(float Tolerance) const {
        return Core::Math::Simd::NearEqual<2>(mValue, _mm_setzero_ps(), Tolerance);
    }

    /**
     * @brief 제곱 길이가 허용 오차 이내에서 1인지 검사합니다.
     */
    bool Core::FVector2::IsNormalized(float Tolerance) const {
        return IsFinite() && std::abs(LengthSquared() - 1.0f) <= Tolerance;
    }

    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     */
    bool Core::FVector2::IsFinite() const {
        return Core::Math::Simd::IsFinite<2>(mValue);
    }

    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     */
    bool Core::FVector2::Equals(const Core::FVector2& Other, float Tolerance) const {
        return Core::Math::Simd::NearEqual<2>(mValue, Other.mValue, Tolerance);
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FVector2 operator*(float Value, const Core::FVector2& Vector) {
        return Vector * Value;
    }

    /**
     * @brief 두 벡터의 외적을 계산합니다.
     */
    float Core::FVector2::Cross(const Core::FVector2& Other) const {
        return x * Other.y - y * Other.x;
    }

    /**
     * @brief 2차원 벡터를 도 단위 각도만큼 회전합니다.
     */
    Core::FVector2 Core::FVector2::Rotated(float Degrees) const {
        const float Radians{Core::Math::ToRadians(Degrees)};
        const float Sine{std::sin(Radians)};
        const float Cosine{std::cos(Radians)};

        return {x * Cosine - y * Sine, x * Sine + y * Cosine};
    }

    /**
     * @brief 유효한 성분을 원래 순서의 일반 배열로 반환합니다.
     */
    std::array<float, 2> Core::FVector2::ToArray() const {
        return {x, y};
    }

}
