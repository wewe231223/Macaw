/**
 * @file FVector4.cpp
 * @brief 16바이트 정렬 SIMD 저장소를 사용하는 동차 4차원 벡터입니다.
 * @ingroup Core
 */

#include "Core/Math/FVector4.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Math/SimdMath.h"

#include <algorithm>
#include <cmath>

namespace Core {

    /**
     * @brief FVector4의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FVector4::FVector4(float X, float Y, float Z, float W)
		: mValue{_mm_set_ps(W, Z, Y, X)} {
    }

    /**
     * @brief FVector4의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FVector4::FVector4(const Core::FVector& Vector, float W)
		: Core::FVector4{Vector.x, Vector.y, Vector.z, W} {
    }

    /**
     * @brief FVector4의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FVector4::FVector4(__m128 Value)
		: mValue{Value} {
    }

    /**
     * @brief 동차 행 벡터에 지정한 4×4 행렬을 곱합니다.
     */
    Core::FVector4 Core::FVector4::operator*(const Core::FMatrix& Matrix) const {
        return Core::FVector4{Core::Math::Simd::Transform(mValue, Matrix.mRows)};
    }

    /**
     * @brief 동차 행 벡터에 지정한 4×4 행렬을 곱합니다.
     */
    Core::FVector4& Core::FVector4::operator*=(const Core::FMatrix& Matrix) {
        *this = *this * Matrix;

        return *this;
    }

    /**
     * @brief 대응 성분을 더한 값을 반환합니다.
     */
    Core::FVector4 Core::FVector4::operator+(const Core::FVector4& Other) const {
        return Core::FVector4{_mm_add_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 대응 성분끼리 뺀 값을 반환합니다.
     */
    Core::FVector4 Core::FVector4::operator-(const Core::FVector4& Other) const {
        return Core::FVector4{_mm_sub_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 대응하는 성분끼리 곱합니다.
     */
    Core::FVector4 Core::FVector4::operator*(const Core::FVector4& Other) const {
        return Core::FVector4{_mm_mul_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 대응 성분끼리 나눈 결과를 반환합니다.
     */
    Core::FVector4 Core::FVector4::operator/(const Core::FVector4& Other) const {
        return Core::FVector4{_mm_div_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 모든 성분의 부호를 반전한 값을 반환합니다.
     */
    Core::FVector4 Core::FVector4::operator-() const {
        return Core::FVector4{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FVector4 Core::FVector4::operator*(float Value) const {
        return Core::FVector4{_mm_mul_ps(mValue, _mm_set1_ps(Value))};
    }

    /**
     * @brief 각 성분을 스칼라 값으로 나눈 결과를 반환합니다.
     */
    Core::FVector4 Core::FVector4::operator/(float Value) const {
        return *this * (1.0f / Value);
    }

    /**
     * @brief 대응 성분을 현재 값에 더합니다.
     */
    Core::FVector4& Core::FVector4::operator+=(const Core::FVector4& Other) {
        *this = *this + Other;

        return *this;
    }

    /**
     * @brief 대응 성분을 현재 값에서 뺍니다.
     */
    Core::FVector4& Core::FVector4::operator-=(const Core::FVector4& Other) {
        *this = *this - Other;

        return *this;
    }

    /**
     * @brief 대응 성분끼리 곱한 결과를 현재 값에 반영합니다.
     */
    Core::FVector4& Core::FVector4::operator*=(const Core::FVector4& Other) {
        *this = *this * Other;

        return *this;
    }

    /**
     * @brief 대응 성분끼리 나누어 현재 값에 반영합니다.
     */
    Core::FVector4& Core::FVector4::operator/=(const Core::FVector4& Other) {
        *this = *this / Other;

        return *this;
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FVector4& Core::FVector4::operator*=(float Value) {
        *this = *this * Value;

        return *this;
    }

    /**
     * @brief 각 성분을 스칼라 값으로 나누어 현재 값에 반영합니다.
     */
    Core::FVector4& Core::FVector4::operator/=(float Value) {
        *this = *this / Value;

        return *this;
    }

    /**
     * @brief 모든 성분이 허용 오차 없이 정확히 같은지 검사합니다.
     */
    bool Core::FVector4::operator==(const Core::FVector4& Other) const {
        return Core::Math::Simd::Equal<4>(mValue, Other.mValue);
    }

    /**
     * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
     */
    float Core::FVector4::Dot(const Core::FVector4& Other) const {
        return _mm_cvtss_f32(Core::Math::Simd::Dot<4>(mValue, Other.mValue));
    }

    /**
     * @brief 성분의 제곱합을 반환합니다.
     */
    float Core::FVector4::LengthSquared() const {
        return Dot(*this);
    }

    /**
     * @brief 유클리드 길이를 반환합니다.
     */
    float Core::FVector4::Length() const {
        return _mm_cvtss_f32(_mm_sqrt_ps(Core::Math::Simd::Dot<4>(mValue, mValue)));
    }

    /**
     * @brief 정규화한 복사본을 반환합니다.
     */
    Core::FVector4 Core::FVector4::Normalized() const {
        Core::FVector4 Result{*this};
        Result.Normalize();

        return Result;
    }

    /**
     * @brief 값을 제자리에서 정규화합니다.
     */
    bool Core::FVector4::Normalize() {
        return Core::Math::Simd::Normalize<4>(mValue);
    }

    /**
     * @brief 두 값 사이 유클리드 거리의 제곱을 구합니다.
     */
    float Core::FVector4::DistanceSquared(const Core::FVector4& Other) const {
        return (*this - Other).LengthSquared();
    }

    /**
     * @brief 두 값 사이 유클리드 거리를 구합니다.
     */
    float Core::FVector4::Distance(const Core::FVector4& Other) const {
        return (*this - Other).Length();
    }

    /**
     * @brief 두 값 사이를 주어진 비율로 선형 보간합니다.
     */
    Core::FVector4 Core::FVector4::Lerp(const Core::FVector4& Other, float Alpha) const {
        return *this + (Other - *this) * Alpha;
    }

    /**
     * @brief 각 성분의 절댓값을 반환합니다.
     */
    Core::FVector4 Core::FVector4::Abs() const {
        return Core::FVector4{_mm_andnot_ps(_mm_set1_ps(-0.0f), mValue)};
    }

    /**
     * @brief 각 성분에서 두 값 중 작은 값을 선택합니다.
     */
    Core::FVector4 Core::FVector4::Min(const Core::FVector4& Other) const {
        return Core::FVector4{_mm_min_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 각 성분에서 두 값 중 큰 값을 선택합니다.
     */
    Core::FVector4 Core::FVector4::Max(const Core::FVector4& Other) const {
        return Core::FVector4{_mm_max_ps(mValue, Other.mValue)};
    }

    /**
     * @brief 각 성분을 지정한 최솟값과 최댓값 사이로 제한합니다.
     */
    Core::FVector4 Core::FVector4::Clamped(const Core::FVector4& Minimum, const Core::FVector4& Maximum) const {
        return Max(Minimum).Min(Maximum);
    }

    /**
     * @brief 방향을 유지하며 벡터 길이를 지정한 상한으로 제한합니다.
     */
    Core::FVector4 Core::FVector4::ClampedLength(float MaximumLength) const {
        const float Limit{std::max(MaximumLength, 0.0f)};

        const double SizeSquared{static_cast<double>(x) * x + static_cast<double>(y) * y + static_cast<double>(z) * z + static_cast<double>(w) * w};

        return SizeSquared > static_cast<double>(Limit) * Limit ? Normalized() * Limit : *this;
    }

    /**
     * @brief 모든 유효 성분이 허용 오차 이내에서 0인지 검사합니다.
     */
    bool Core::FVector4::IsNearlyZero(float Tolerance) const {
        return Core::Math::Simd::NearEqual<4>(mValue, _mm_setzero_ps(), Tolerance);
    }

    /**
     * @brief 제곱 길이가 허용 오차 이내에서 1인지 검사합니다.
     */
    bool Core::FVector4::IsNormalized(float Tolerance) const {
        return IsFinite() && std::abs(LengthSquared() - 1.0f) <= Tolerance;
    }

    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     */
    bool Core::FVector4::IsFinite() const {
        return Core::Math::Simd::IsFinite<4>(mValue);
    }

    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     */
    bool Core::FVector4::Equals(const Core::FVector4& Other, float Tolerance) const {
        return Core::Math::Simd::NearEqual<4>(mValue, Other.mValue, Tolerance);
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FVector4 operator*(float Value, const Core::FVector4& Vector) {
        return Vector * Value;
    }

    /**
     * @brief 앞 세 성분을 3차원 벡터로 반환합니다.
     */
    Core::FVector Core::FVector4::ToVector3() const {
        return {x, y, z};
    }

    /**
     * @brief 유효한 동차 W로 XYZ를 나누어 3차원 좌표를 구합니다.
     */
    bool Core::FVector4::TryPerspectiveDivide(Core::FVector& Result, float Tolerance) const {
        if (!IsFinite() || std::abs(w) <= std::abs(Tolerance)) {
            return false;
        }

        const Core::FVector Candidate{x / w, y / w, z / w};

        if (!Candidate.IsFinite()) {
            return false;
        }

        Result = Candidate;

        return true;
    }

    /**
     * @brief 유효한 성분을 원래 순서의 일반 배열로 반환합니다.
     */
    std::array<float, 4> Core::FVector4::ToArray() const {
        return {x, y, z, w};
    }

}
