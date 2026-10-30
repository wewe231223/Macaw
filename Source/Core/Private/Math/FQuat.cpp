/**
 * @file FQuat.cpp
 * @brief 회전의 합성과 보간에 사용하는 단위 쿼터니언입니다.
 * @ingroup Core
 */

#include "Core/Math/FQuat.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FVector.h"
#include "Math/SimdMath.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Core {

    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FQuat::FQuat()
		: mValue{_mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f)} {
    }

    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FQuat::FQuat(float X, float Y, float Z, float W)
		: mValue{_mm_set_ps(W, Z, Y, X)} {
    }

    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FQuat::FQuat(__m128 Value)
		: mValue{Value} {
    }

    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FQuat::FQuat(const Core::FRotator& Rotation) {
        *this = Rotation.Quaternion();
    }

    /**
     * @brief FQuat의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FQuat::FQuat(const Core::FVector& Axis, float Radians)
		: Core::FQuat{} {
        Core::FVector Direction{Axis};

        if (Direction.Normalize()) {
            const float HalfAngle{Radians * 0.5f};
            const __m128 Vector{_mm_mul_ps(Direction.mValue, _mm_set1_ps(std::sin(HalfAngle)))};
            *this = Core::FQuat{_mm_add_ps(Vector, _mm_set_ps(std::cos(HalfAngle), 0.0f, 0.0f, 0.0f))};
        }
    }

    /**
     * @brief 두 쿼터니언의 회전 합성 곱을 계산합니다.
     */
    Core::FQuat Core::FQuat::operator*(const Core::FQuat& Other) const {
        return Core::FQuat{Core::Math::Simd::MultiplyQuaternion(mValue, Other.mValue)};
    }

    /**
     * @brief 두 쿼터니언의 회전 합성 곱을 계산합니다.
     */
    Core::FQuat& Core::FQuat::operator*=(const Core::FQuat& Other) {
        *this = *this * Other;

        return *this;
    }

    /**
     * @brief 현재 쿼터니언 회전을 벡터에 적용합니다.
     */
    Core::FVector Core::FQuat::operator*(const Core::FVector& Vector) const {
        return RotateVector(Vector);
    }

    /**
     * @brief 성분의 부호를 반전하거나 상대 값을 뺍니다.
     */
    Core::FQuat Core::FQuat::operator-() const {
        return Core::FQuat{_mm_xor_ps(mValue, _mm_set1_ps(-0.0f))};
    }

    /**
     * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
     */
    bool Core::FQuat::operator==(const Core::FQuat& Other) const {
        return Core::Math::Simd::Equal<4>(mValue, Other.mValue);
    }

    /**
     * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
     */
    float Core::FQuat::Dot(const Core::FQuat& Other) const {
        return _mm_cvtss_f32(Core::Math::Simd::Dot<4>(mValue, Other.mValue));
    }

    /**
     * @brief 성분의 제곱합을 반환합니다.
     */
    float Core::FQuat::LengthSquared() const {
        return Dot(*this);
    }

    /**
     * @brief 정규화한 복사본을 반환합니다.
     */
    Core::FQuat Core::FQuat::Normalized() const {
        Core::FQuat Result{*this};
        Result.Normalize();

        return Result;
    }

    /**
     * @brief 유클리드 길이를 반환합니다.
     */
    float Core::FQuat::Length() const {
        return std::sqrt(LengthSquared());
    }

    /**
     * @brief 값을 제자리에서 정규화합니다.
     */
    bool Core::FQuat::Normalize() {
        if (!Core::Math::Simd::Normalize<4>(mValue)) {
            *this = Core::FQuat{};
            return false;
        }

        return true;
    }

    /**
     * @brief 쿼터니언 벡터 성분의 부호를 반전한 켤레를 반환합니다.
     */
    Core::FQuat Core::FQuat::Conjugate() const {
        return Core::FQuat{_mm_xor_ps(mValue, _mm_set_ps(0.0f, -0.0f, -0.0f, -0.0f))};
    }

    /**
     * @brief 최단 회전 경로를 선택하여 선형 보간한 뒤 정규화합니다.
     */
    Core::FQuat Core::FQuat::Nlerp(const Core::FQuat& Other, float Alpha) const {
        const Core::FQuat Start{Normalized()};
        Core::FQuat End{Other.Normalized()};

        if (Start.Dot(End) < 0.0f) {
            End = -End;
        }

        return Core::FQuat{_mm_add_ps(_mm_mul_ps(Start.mValue, _mm_set1_ps(1.0f - Alpha)), _mm_mul_ps(End.mValue, _mm_set1_ps(Alpha)))}.Normalized();
    }

    /**
     * @brief 최단 회전 경로를 따라 구면 선형 보간합니다.
     */
    Core::FQuat Core::FQuat::Slerp(const Core::FQuat& Other, float Alpha) const {
        const Core::FQuat Start{Normalized()};
        Core::FQuat End{Other.Normalized()};
        float Cosine{Start.Dot(End)};

        if (Cosine < 0.0f) {
            End = -End;
            Cosine = -Cosine;
        }

        float StartWeight{1.0f - Alpha};
        float EndWeight{Alpha};

        if (Cosine <= 0.9995f) {
            const float Angle{std::acos(std::clamp(Cosine, 0.0f, 1.0f))};
            const float Sine{std::sin(Angle)};
            StartWeight = std::sin((1.0f - Alpha) * Angle) / Sine;
            EndWeight = std::sin(Alpha * Angle) / Sine;
        }

        return Core::FQuat{_mm_add_ps(_mm_mul_ps(Start.mValue, _mm_set1_ps(StartWeight)), _mm_mul_ps(End.mValue, _mm_set1_ps(EndWeight)))}.Normalized();
    }

    /**
     * @brief 두 쿼터니언 회전 사이의 각도 차이를 라디안으로 구합니다.
     */
    float Core::FQuat::AngularDistance(const Core::FQuat& Other) const {
        const Core::FQuat Difference{Other.Normalized() * Normalized().Conjugate()};
        const float Sine{Core::FVector{Difference.x, Difference.y, Difference.z}.Length()};

        return 2.0f * std::atan2(Sine, std::abs(Difference.w));
    }

    /**
     * @brief 쿼터니언 회전을 단위 축과 라디안 각도로 분해합니다.
     */
    void Core::FQuat::ToAxisAngle(Core::FVector& Axis, float& Radians) const {
        Core::FQuat Unit{Normalized()};

        if (Unit.w < 0.0f) {
            Unit = -Unit;
        }

        const Core::FVector Vector{Unit.x, Unit.y, Unit.z};
        const float Sine{Vector.Length()};
        Axis = Sine > 0.000001f ? Vector / Sine : Core::FVector{1.0f, 0.0f, 0.0f};
        Radians = 2.0f * std::atan2(Sine, Unit.w);
    }

    /**
     * @brief 회전된 전방 단위 벡터를 반환합니다.
     */
    Core::FVector Core::FQuat::ForwardVector() const {
        return RotateVector({1.0f, 0.0f, 0.0f});
    }

    /**
     * @brief 회전된 우측 단위 벡터를 반환합니다.
     */
    Core::FVector Core::FQuat::RightVector() const {
        return RotateVector({0.0f, 1.0f, 0.0f});
    }

    /**
     * @brief 회전된 상방 단위 벡터를 반환합니다.
     */
    Core::FVector Core::FQuat::UpVector() const {
        return RotateVector({0.0f, 0.0f, 1.0f});
    }

    /**
     * @brief 켤레를 제곱 길이로 나눈 역쿼터니언을 반환합니다.
     */
    Core::FQuat Core::FQuat::Inverse() const {
        const __m128 Value{mValue};
        const __m128 SizeSquared{Core::Math::Simd::Dot<4>(Value, Value)};

        if (!Core::Math::Simd::IsFinite<4>(Value) || _mm_cvtss_f32(SizeSquared) <= 0.000000000001f) {
            return {0.0f, 0.0f, 0.0f, 0.0f};
        }

        return Core::FQuat{_mm_div_ps(Conjugate().mValue, SizeSquared)};
    }

    /**
     * @brief 쿼터니언 회전을 도 단위 오일러 각으로 변환합니다.
     */
    Core::FRotator Core::FQuat::Rotator() const {
        const Core::FQuat Unit{Normalized()};
        const double Length{std::sqrt(static_cast<double>(Unit.x) * Unit.x + static_cast<double>(Unit.y) * Unit.y + static_cast<double>(Unit.z) * Unit.z + static_cast<double>(Unit.w) * Unit.w)};
        const double X{Unit.x / Length};
        const double Y{Unit.y / Length};
        const double Z{Unit.z / Length};
        const double W{Unit.w / Length};
        const double Singularity{Z * X - W * Y};
        const double YawY{2.0 * (W * Z + X * Y)};
        const double YawX{1.0 - 2.0 * (Y * Y + Z * Z)};
        constexpr double DegreesPerRadian{180.0 / std::numbers::pi_v<double>};

        Core::FRotator Result{};
        Result.yaw = static_cast<float>(std::atan2(YawY, YawX) * DegreesPerRadian);

        if (Singularity < -0.499999999999) {
            Result.pitch = -90.0f;
            Result.roll = static_cast<float>(-Result.yaw - 2.0 * std::atan2(X, W) * DegreesPerRadian);
        } else if (Singularity > 0.499999999999) {
            Result.pitch = 90.0f;
            Result.roll = static_cast<float>(Result.yaw - 2.0 * std::atan2(X, W) * DegreesPerRadian);
        } else {
            Result.pitch = static_cast<float>(std::asin(std::clamp(2.0 * Singularity, -1.0, 1.0)) * DegreesPerRadian);
            Result.roll = static_cast<float>(std::atan2(-2.0 * (W * X + Y * Z), 1.0 - 2.0 * (X * X + Y * Y)) * DegreesPerRadian);
        }

        return Result.Normalized();
    }

    /**
     * @brief 현재 회전을 벡터에 적용합니다.
     */
    Core::FVector Core::FQuat::RotateVector(const Core::FVector& Vector) const {
        return Core::FVector{Core::Math::Simd::RotateVector(Vector.mValue, mValue)};
    }

    /**
     * @brief 현재 회전의 역회전을 벡터에 적용합니다.
     */
    Core::FVector Core::FQuat::UnrotateVector(const Core::FVector& Vector) const {
        return Conjugate().RotateVector(Vector);
    }

    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     */
    bool Core::FQuat::IsFinite() const {
        return Core::Math::Simd::IsFinite<4>(mValue);
    }

    /**
     * @brief 제곱 길이가 허용 오차 이내에서 1인지 검사합니다.
     */
    bool Core::FQuat::IsNormalized(float Tolerance) const {
        return IsFinite() && std::abs(LengthSquared() - 1.0f) <= Tolerance;
    }

    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     */
    bool Core::FQuat::Equals(const Core::FQuat& Other, float Tolerance) const {
        const __m128 Value{mValue};
        const __m128 Compared{Other.mValue};

        return Core::Math::Simd::NearEqual<4>(Value, Compared, Tolerance) || Core::Math::Simd::NearEqual<4>(Value, _mm_xor_ps(Compared, _mm_set1_ps(-0.0f)), Tolerance);
    }

}
