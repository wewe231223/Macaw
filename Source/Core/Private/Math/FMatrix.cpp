/**
 * @file FMatrix.cpp
 * @brief 행 벡터 변환 규약을 사용하는 16바이트 정렬 4×4 행렬입니다.
 * @ingroup Core
 */

#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/MathUtility.h"
#include "Math/SimdMath.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Core {

    /**
     * @brief FMatrix의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Core::FMatrix::FMatrix() {
        mRows[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
        mRows[1] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
        mRows[2] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
        mRows[3] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
    }

    /**
     * @brief 항등행렬을 생성합니다.
     */
    Core::FMatrix Core::FMatrix::Identity() {
        return {};
    }

    /**
     * @brief 이동 성분을 갖는 변환 행렬을 생성합니다.
     */
    Core::FMatrix Core::FMatrix::Translation(const Core::FVector& Value) {
        Core::FMatrix Result{};
        Result.mRows[3] = _mm_set_ps(1.0f, Value.z, Value.y, Value.x);

        return Result;
    }

    /**
     * @brief 축별 스케일 행렬을 생성합니다.
     */
    Core::FMatrix Core::FMatrix::Scale(const Core::FVector& Value) {
        Core::FMatrix Result{};
        Result.mRows[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, Value.x);
        Result.mRows[1] = _mm_set_ps(0.0f, 0.0f, Value.y, 0.0f);
        Result.mRows[2] = _mm_set_ps(0.0f, Value.z, 0.0f, 0.0f);

        return Result;
    }

    /**
     * @brief 오일러 회전을 회전 행렬로 변환합니다.
     */
    Core::FMatrix Core::FMatrix::Rotation(const Core::FRotator& Value) {
        return Rotation(Value.Quaternion());
    }

    /**
     * @brief 쿼터니언 회전을 회전 행렬로 변환합니다.
     */
    Core::FMatrix Core::FMatrix::Rotation(const Core::FQuat& Value) {
        const __m128 Quaternion{Value.mValue};
        const __m128 Twice{_mm_add_ps(Quaternion, Quaternion)};
        const __m128 ProductX{_mm_mul_ps(Core::Math::Simd::Splat<0>(Twice), Quaternion)};
        const __m128 ProductY{_mm_mul_ps(Core::Math::Simd::Splat<1>(Twice), Quaternion)};
        const __m128 ProductZ{_mm_mul_ps(Core::Math::Simd::Splat<2>(Twice), Quaternion)};

        const float Xx{Core::Math::Simd::Extract<0>(ProductX)};
        const float Xy{Core::Math::Simd::Extract<1>(ProductX)};
        const float Xz{Core::Math::Simd::Extract<2>(ProductX)};
        const float Xw{Core::Math::Simd::Extract<3>(ProductX)};
        const float Yy{Core::Math::Simd::Extract<1>(ProductY)};
        const float Yz{Core::Math::Simd::Extract<2>(ProductY)};
        const float Yw{Core::Math::Simd::Extract<3>(ProductY)};
        const float Zz{Core::Math::Simd::Extract<2>(ProductZ)};
        const float Zw{Core::Math::Simd::Extract<3>(ProductZ)};

        Core::FMatrix Result{};
        Result.mRows[0] = _mm_set_ps(0.0f, Xz - Yw, Xy + Zw, 1.0f - Yy - Zz);
        Result.mRows[1] = _mm_set_ps(0.0f, Yz + Xw, 1.0f - Xx - Zz, Xy - Zw);
        Result.mRows[2] = _mm_set_ps(0.0f, 1.0f - Xx - Yy, Yz - Xw, Xz + Yw);

        return Result;
    }

    /**
     * @brief 지정한 축을 중심으로 라디안 단위 회전 행렬을 생성합니다.
     */
    Core::FMatrix Core::FMatrix::RotationAxis(const Core::FVector& Axis, float Radians) {
        return Rotation(Core::FQuat{Axis, Radians});
    }

    /**
     * @brief 위치, 회전과 스케일을 행 벡터 규약의 행렬로 합성합니다.
     */
    Core::FMatrix Core::FMatrix::FromTransform(const Core::FTransform& Transform) {
        const __m128 Scale{Transform.mScale.mValue};
        Core::FMatrix Result{Rotation(Transform.mRotation)};
        Result.mRows[0] = _mm_mul_ps(Result.mRows[0], Core::Math::Simd::Splat<0>(Scale));
        Result.mRows[1] = _mm_mul_ps(Result.mRows[1], Core::Math::Simd::Splat<1>(Scale));
        Result.mRows[2] = _mm_mul_ps(Result.mRows[2], Core::Math::Simd::Splat<2>(Scale));
        Result.mRows[3] = _mm_add_ps(Transform.mLocation.mValue, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));

        return Result;
    }

    /**
     * @brief 카메라 위치, 전방과 상방으로 뷰 행렬을 생성합니다.
     */
    Core::FMatrix Core::FMatrix::LookTo(const Core::FVector& Position, const Core::FVector& Forward, const Core::FVector& Up) {
        const Core::FVector Direction{Forward.Normalized()};
        const Core::FVector Right{Up.Cross(Direction).Normalized()};
        const Core::FVector Vertical{Direction.Cross(Right)};

        Core::FMatrix Result{};
        Result.mRows[0] = _mm_set_ps(0.0f, Direction.x, Vertical.x, Right.x);
        Result.mRows[1] = _mm_set_ps(0.0f, Direction.y, Vertical.y, Right.y);
        Result.mRows[2] = _mm_set_ps(0.0f, Direction.z, Vertical.z, Right.z);
        Result.mRows[3] = _mm_set_ps(1.0f, -Direction.Dot(Position), -Vertical.Dot(Position), -Right.Dot(Position));

        return Result;
    }

    /**
     * @brief 수직 시야각과 종횡비로 Direct3D 깊이 범위의 원근 투영 행렬을 생성합니다.
     */
    Core::FMatrix Core::FMatrix::Perspective(float FieldOfView, float Aspect, float NearPlane, float FarPlane) {
        const float Height{1.0f / std::tan(Core::Math::ToRadians(FieldOfView) * 0.5f)};
        const float Width{Height / Aspect};
        const float Depth{FarPlane / (FarPlane - NearPlane)};

        Core::FMatrix Result{};
        Result.mRows[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, Width);
        Result.mRows[1] = _mm_set_ps(0.0f, 0.0f, Height, 0.0f);
        Result.mRows[2] = _mm_set_ps(1.0f, Depth, 0.0f, 0.0f);
        Result.mRows[3] = _mm_set_ps(0.0f, -NearPlane * Depth, 0.0f, 0.0f);

        return Result;
    }

    Core::FMatrix Core::FMatrix::Orthographic(float Width, float Height, float NearPlane, float FarPlane) {
        Core::FMatrix Result{};
        Result(0, 0) = 2.0f / Width;
        Result(1, 1) = 2.0f / Height;
        Result(2, 2) = 1.0f / (FarPlane - NearPlane);
        Result(3, 2) = -NearPlane / (FarPlane - NearPlane);

        return Result;
    }

    /**
     * @brief 대응 성분을 더한 값을 반환합니다.
     */
    Core::FMatrix Core::FMatrix::operator+(const Core::FMatrix& Other) const {
        Core::FMatrix Result{};

        for (Core::Int32 Row{}; Row < 4; ++Row) {
            Result.mRows[Row] = _mm_add_ps(mRows[Row], Other.mRows[Row]);
        }

        return Result;
    }

    /**
     * @brief 대응 성분끼리 뺀 값을 반환합니다.
     */
    Core::FMatrix Core::FMatrix::operator-(const Core::FMatrix& Other) const {
        Core::FMatrix Result{};

        for (Core::Int32 Row{}; Row < 4; ++Row) {
            Result.mRows[Row] = _mm_sub_ps(mRows[Row], Other.mRows[Row]);
        }

        return Result;
    }

    /**
     * @brief 현재 행렬 뒤에 상대 행렬을 적용하는 행 벡터 규약의 곱을 계산합니다.
     */
    Core::FMatrix Core::FMatrix::operator*(const Core::FMatrix& Other) const {
        const __m128 OtherX{Other.mRows[0]};
        const __m128 OtherY{Other.mRows[1]};
        const __m128 OtherZ{Other.mRows[2]};
        const __m128 OtherW{Other.mRows[3]};
        Core::FMatrix Result{};

        for (Core::Int32 Row{}; Row < 4; ++Row) {
            const __m128 Value{mRows[Row]};
            const __m128 X{_mm_mul_ps(Core::Math::Simd::Splat<0>(Value), OtherX)};
            const __m128 Y{_mm_mul_ps(Core::Math::Simd::Splat<1>(Value), OtherY)};
            const __m128 Z{_mm_mul_ps(Core::Math::Simd::Splat<2>(Value), OtherZ)};
            const __m128 W{_mm_mul_ps(Core::Math::Simd::Splat<3>(Value), OtherW)};

            Result.mRows[Row] = _mm_add_ps(_mm_add_ps(X, Y), _mm_add_ps(Z, W));
        }

        return Result;
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FMatrix Core::FMatrix::operator*(float Value) const {
        const __m128 Scale{_mm_set1_ps(Value)};
        Core::FMatrix Result{};

        for (Core::Int32 Row{}; Row < 4; ++Row) {
            Result.mRows[Row] = _mm_mul_ps(mRows[Row], Scale);
        }

        return Result;
    }

    /**
     * @brief 각 성분을 스칼라 값으로 나눈 결과를 반환합니다.
     */
    Core::FMatrix Core::FMatrix::operator/(float Value) const {
        return *this * (1.0f / Value);
    }

    /**
     * @brief 대응 성분을 현재 값에 더합니다.
     */
    Core::FMatrix& Core::FMatrix::operator+=(const Core::FMatrix& Other) {
        *this = *this + Other;

        return *this;
    }

    /**
     * @brief 대응 성분을 현재 값에서 뺍니다.
     */
    Core::FMatrix& Core::FMatrix::operator-=(const Core::FMatrix& Other) {
        *this = *this - Other;

        return *this;
    }

    /**
     * @brief 현재 행렬 뒤에 상대 행렬을 적용하는 행 벡터 규약의 곱을 계산합니다.
     */
    Core::FMatrix& Core::FMatrix::operator*=(const Core::FMatrix& Other) {
        *this = *this * Other;

        return *this;
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FMatrix& Core::FMatrix::operator*=(float Value) {
        *this = *this * Value;

        return *this;
    }

    /**
     * @brief 각 성분을 스칼라 값으로 나누어 현재 값에 반영합니다.
     */
    Core::FMatrix& Core::FMatrix::operator/=(float Value) {
        *this = *this / Value;

        return *this;
    }

    /**
     * @brief 모든 성분이 허용 오차 없이 정확히 같은지 검사합니다.
     */
    bool Core::FMatrix::operator==(const Core::FMatrix& Other) const {
        for (Core::Int32 Row{}; Row < 4; ++Row) {
            if (!Core::Math::Simd::Equal<4>(mRows[Row], Other.mRows[Row])) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief 동차 W=1인 위치를 변환하고 결과 W로 나눕니다.
     */
    Core::FVector Core::FMatrix::TransformPosition(const Core::FVector& Position) const {
        const __m128 Homogeneous{_mm_add_ps(Position.mValue, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f))};
        const __m128 Result{Core::Math::Simd::Transform(Homogeneous, mRows)};

        return Core::FVector{_mm_div_ps(Result, Core::Math::Simd::Splat<3>(Result))};
    }

    /**
     * @brief 방향 벡터에 스케일과 회전을 적용합니다.
     */
    Core::FVector Core::FMatrix::TransformVector(const Core::FVector& Vector) const {
        return Core::FVector{Core::Math::Simd::Transform(Vector.mValue, mRows)};
    }

    /**
     * @brief 행렬에서 지정한 행의 XYZ 축 성분을 반환합니다.
     */
    Core::FVector Core::FMatrix::GetAxis(Core::Int32 Index) const {
        return Core::FVector{mRows[Index]};
    }

    /**
     * @brief 회전 값을 반환합니다.
     */
    Core::FRotator Core::FMatrix::GetRotation() const {
        return GetQuaternion().Rotator();
    }

    /**
     * @brief 행렬에서 추출한 쿼터니언 회전을 반환합니다.
     */
    Core::FQuat Core::FMatrix::GetQuaternion() const {
        Core::FMatrix Rotation{*this};
        std::array<float, 3> Scales{};

        for (Core::Int32 Row{}; Row < 3; ++Row) {
            const __m128 Axis{mRows[Row]};
            const __m128 SizeSquared{Core::Math::Simd::Dot<3>(Axis, Axis)};

            if (!Core::Math::Simd::IsFinite<3>(Axis) || _mm_cvtss_f32(SizeSquared) <= std::numeric_limits<float>::min()) {
                return {};
            }

            const __m128 Size{_mm_sqrt_ps(SizeSquared)};
            Scales[Row] = _mm_cvtss_f32(Size);
            Rotation.mRows[Row] = _mm_div_ps(Axis, Size);
        }

        const float Determinant{_mm_cvtss_f32(Core::Math::Simd::Dot<3>(Core::Math::Simd::Cross3(Rotation.mRows[0], Rotation.mRows[1]), Rotation.mRows[2]))};

        if (!std::isfinite(Determinant) || std::abs(Determinant * Determinant - 1.0f) > 0.0001f) {
            return {};
        }

        if (Determinant < 0.0f) {
            const auto Largest{std::max_element(Scales.begin(), Scales.end())};
            const auto Row{static_cast<std::size_t>(Largest - Scales.begin())};
            Rotation.mRows[Row] = _mm_xor_ps(Rotation.mRows[Row], _mm_set1_ps(-0.0f));
        }

        const float Trace{Rotation.mRows[0].m128_f32[0] + Rotation.mRows[1].m128_f32[1] + Rotation.mRows[2].m128_f32[2]};
        Core::FQuat Result{};

        if (Trace > 0.0f) {
            const float Scale{2.0f * std::sqrt(Trace + 1.0f)};
            Result = {(Rotation.mRows[1].m128_f32[2] - Rotation.mRows[2].m128_f32[1]) / Scale, (Rotation.mRows[2].m128_f32[0] - Rotation.mRows[0].m128_f32[2]) / Scale, (Rotation.mRows[0].m128_f32[1] - Rotation.mRows[1].m128_f32[0]) / Scale, Scale * 0.25f};
        } else if (Rotation.mRows[0].m128_f32[0] > Rotation.mRows[1].m128_f32[1] && Rotation.mRows[0].m128_f32[0] > Rotation.mRows[2].m128_f32[2]) {
            const float Scale{2.0f * std::sqrt(1.0f + Rotation.mRows[0].m128_f32[0] - Rotation.mRows[1].m128_f32[1] - Rotation.mRows[2].m128_f32[2])};
            Result = {Scale * 0.25f, (Rotation.mRows[0].m128_f32[1] + Rotation.mRows[1].m128_f32[0]) / Scale, (Rotation.mRows[0].m128_f32[2] + Rotation.mRows[2].m128_f32[0]) / Scale, (Rotation.mRows[1].m128_f32[2] - Rotation.mRows[2].m128_f32[1]) / Scale};
        } else if (Rotation.mRows[1].m128_f32[1] > Rotation.mRows[2].m128_f32[2]) {
            const float Scale{2.0f * std::sqrt(1.0f + Rotation.mRows[1].m128_f32[1] - Rotation.mRows[0].m128_f32[0] - Rotation.mRows[2].m128_f32[2])};
            Result = {(Rotation.mRows[0].m128_f32[1] + Rotation.mRows[1].m128_f32[0]) / Scale, Scale * 0.25f, (Rotation.mRows[1].m128_f32[2] + Rotation.mRows[2].m128_f32[1]) / Scale, (Rotation.mRows[2].m128_f32[0] - Rotation.mRows[0].m128_f32[2]) / Scale};
        } else {
            const float Scale{2.0f * std::sqrt(1.0f + Rotation.mRows[2].m128_f32[2] - Rotation.mRows[0].m128_f32[0] - Rotation.mRows[1].m128_f32[1])};
            Result = {(Rotation.mRows[0].m128_f32[2] + Rotation.mRows[2].m128_f32[0]) / Scale, (Rotation.mRows[1].m128_f32[2] + Rotation.mRows[2].m128_f32[1]) / Scale, Scale * 0.25f, (Rotation.mRows[0].m128_f32[1] - Rotation.mRows[1].m128_f32[0]) / Scale};
        }

        return Result.Normalized();
    }

    /**
     * @brief 역행렬의 유효성을 검사하고 계산 가능한 경우 결과에 기록합니다.
     */
    bool Core::FMatrix::TryInverse(Core::FMatrix& Result) const {
        Core::FMatrix Working{*this};
        Core::FMatrix Inverse{};
        double Determinant{1.0};

        if (!IsFinite()) {
            return false;
        }

        for (Core::Int32 Column{}; Column < 4; ++Column) {
            Core::Int32 Pivot{Column};

            for (Core::Int32 Row{Column + 1}; Row < 4; ++Row) {
                if (std::abs(Working.mRows[Row].m128_f32[Column]) > std::abs(Working.mRows[Pivot].m128_f32[Column])) {
                    Pivot = Row;
                }
            }

            const float Divisor{Working.mRows[Pivot].m128_f32[Column]};

            if (std::abs(Divisor) <= std::numeric_limits<float>::min()) {
                return false;
            }

            if (Pivot != Column) {
                const __m128 WorkingRow{Working.mRows[Column]};
                const __m128 InverseRow{Inverse.mRows[Column]};

                Working.mRows[Column] = Working.mRows[Pivot];
                Inverse.mRows[Column] = Inverse.mRows[Pivot];
                Working.mRows[Pivot] = WorkingRow;
                Inverse.mRows[Pivot] = InverseRow;

                Determinant = -Determinant;
            }

            Determinant *= Divisor;

            const __m128 Denominator{_mm_set1_ps(Divisor)};
            const __m128 WorkingPivot{_mm_div_ps(Working.mRows[Column], Denominator)};
            const __m128 InversePivot{_mm_div_ps(Inverse.mRows[Column], Denominator)};
            Working.mRows[Column] = WorkingPivot;
            Inverse.mRows[Column] = InversePivot;

            for (Core::Int32 Row{}; Row < 4; ++Row) {
                if (Row != Column) {
                    const __m128 Factor{_mm_set1_ps(Working.mRows[Row].m128_f32[Column])};
                    Working.mRows[Row] = _mm_sub_ps(Working.mRows[Row], _mm_mul_ps(Factor, WorkingPivot));
                    Inverse.mRows[Row] = _mm_sub_ps(Inverse.mRows[Row], _mm_mul_ps(Factor, InversePivot));
                }
            }
        }

        if (!std::isfinite(Determinant) || std::abs(Determinant) < 0.00000001) {
            return false;
        }

        if (!Inverse.IsFinite()) {
            return false;
        }

        Result = Inverse;

        return true;
    }

    /**
     * @brief 행과 열을 교환한 전치행렬을 반환합니다.
     */
    Core::FMatrix Core::FMatrix::Transposed() const {
        Core::FMatrix Result{*this};
        _MM_TRANSPOSE4_PS(Result.mRows[0], Result.mRows[1], Result.mRows[2], Result.mRows[3]);

        return Result;
    }

    /**
     * @brief 4×4 행렬의 행렬식을 구합니다.
     */
    float Core::FMatrix::Determinant() const {
        constexpr std::array<std::array<Core::Int32, 2>, 6> Columns{{{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}}};
        std::array<double, 6> Upper{};
        std::array<double, 6> Lower{};

        for (std::size_t Index{}; Index < Columns.size(); ++Index) {
            const Core::Int32 A{Columns[Index][0]};
            const Core::Int32 B{Columns[Index][1]};
            Upper[Index] = static_cast<double>(mRows[0].m128_f32[A]) * mRows[1].m128_f32[B] - static_cast<double>(mRows[0].m128_f32[B]) * mRows[1].m128_f32[A];
            Lower[Index] = static_cast<double>(mRows[2].m128_f32[A]) * mRows[3].m128_f32[B] - static_cast<double>(mRows[2].m128_f32[B]) * mRows[3].m128_f32[A];
        }

        return static_cast<float>(Upper[0] * Lower[5] - Upper[1] * Lower[4] + Upper[2] * Lower[3] + Upper[3] * Lower[2] - Upper[4] * Lower[1] + Upper[5] * Lower[0]);
    }

    /**
     * @brief 행렬의 이동 성분을 반환합니다.
     */
    Core::FVector Core::FMatrix::Origin() const {
        return Core::FVector{mRows[3]};
    }

    /**
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     */
    bool Core::FMatrix::IsFinite() const {
        for (Core::Int32 Row{}; Row < 4; ++Row) {
            if (!Core::Math::Simd::IsFinite<4>(mRows[Row])) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief 현재 변환이 허용 오차 이내에서 항등 변환인지 검사합니다.
     */
    bool Core::FMatrix::IsIdentity(float Tolerance) const {
        return Equals(Core::FMatrix{}, Tolerance);
    }

    /**
     * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
     */
    bool Core::FMatrix::Equals(const Core::FMatrix& Other, float Tolerance) const {
        for (Core::Int32 Row{}; Row < 4; ++Row) {
            if (!Core::Math::Simd::NearEqual<4>(mRows[Row], Other.mRows[Row], Tolerance)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief 각 성분에 스칼라 값을 곱합니다.
     */
    Core::FMatrix operator*(float Value, const Core::FMatrix& Matrix) {
        return Matrix * Value;
    }

    /**
     * @brief 지정한 행과 열의 행렬 성분을 참조합니다.
     */
    float& Core::FMatrix::operator()(Core::Int32 Row, Core::Int32 Column) {
        return mRows[Row].m128_f32[Column];
    }

    /**
     * @brief 지정한 행과 열의 행렬 성분을 참조합니다.
     */
    const float& Core::FMatrix::operator()(Core::Int32 Row, Core::Int32 Column) const {
        return mRows[Row].m128_f32[Column];
    }

}
