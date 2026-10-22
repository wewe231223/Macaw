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

using namespace Math::Simd;

FMatrix::FMatrix() {
    mRows[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
    mRows[1] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
    mRows[2] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
    mRows[3] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
}

FMatrix FMatrix::Identity() {
    return {};
}

FMatrix FMatrix::Translation(const FVector& Value) {
    FMatrix Result{};
    Result.mRows[3] = _mm_set_ps(1.0f, Value.z, Value.y, Value.x);

    return Result;
}

FMatrix FMatrix::Scale(const FVector& Value) {
    FMatrix Result{};
    Result.mRows[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, Value.x);
    Result.mRows[1] = _mm_set_ps(0.0f, 0.0f, Value.y, 0.0f);
    Result.mRows[2] = _mm_set_ps(0.0f, Value.z, 0.0f, 0.0f);

    return Result;
}

FMatrix FMatrix::Rotation(const FRotator& Value) {
    return Rotation(Value.Quaternion());
}

FMatrix FMatrix::Rotation(const FQuat& Value) {
    const __m128 Quaternion{Value.mValue};
    const __m128 Twice{_mm_add_ps(Quaternion, Quaternion)};
    const __m128 ProductX{_mm_mul_ps(Splat<0>(Twice), Quaternion)};
    const __m128 ProductY{_mm_mul_ps(Splat<1>(Twice), Quaternion)};
    const __m128 ProductZ{_mm_mul_ps(Splat<2>(Twice), Quaternion)};

    const float Xx{Extract<0>(ProductX)};
    const float Xy{Extract<1>(ProductX)};
    const float Xz{Extract<2>(ProductX)};
    const float Xw{Extract<3>(ProductX)};
    const float Yy{Extract<1>(ProductY)};
    const float Yz{Extract<2>(ProductY)};
    const float Yw{Extract<3>(ProductY)};
    const float Zz{Extract<2>(ProductZ)};
    const float Zw{Extract<3>(ProductZ)};

    FMatrix Result{};
    Result.mRows[0] = _mm_set_ps(0.0f, Xz - Yw, Xy + Zw, 1.0f - Yy - Zz);
    Result.mRows[1] = _mm_set_ps(0.0f, Yz + Xw, 1.0f - Xx - Zz, Xy - Zw);
    Result.mRows[2] = _mm_set_ps(0.0f, 1.0f - Xx - Yy, Yz - Xw, Xz + Yw);

    return Result;
}

FMatrix FMatrix::RotationAxis(const FVector& Axis, float Radians) {
    return Rotation(FQuat{Axis, Radians});
}

FMatrix FMatrix::FromTransform(const FTransform& Transform) {
    const __m128 Scale{Transform.mScale.mValue};
    FMatrix Result{Rotation(Transform.mRotation)};
    Result.mRows[0] = _mm_mul_ps(Result.mRows[0], Splat<0>(Scale));
    Result.mRows[1] = _mm_mul_ps(Result.mRows[1], Splat<1>(Scale));
    Result.mRows[2] = _mm_mul_ps(Result.mRows[2], Splat<2>(Scale));
    Result.mRows[3] = _mm_add_ps(Transform.mLocation.mValue, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f));

    return Result;
}

FMatrix FMatrix::LookTo(const FVector& Position, const FVector& Forward, const FVector& Up) {
    const FVector Direction{Forward.Normalized()};
    const FVector Right{Up.Cross(Direction).Normalized()};
    const FVector Vertical{Direction.Cross(Right)};

    FMatrix Result{};
    Result.mRows[0] = _mm_set_ps(0.0f, Direction.x, Vertical.x, Right.x);
    Result.mRows[1] = _mm_set_ps(0.0f, Direction.y, Vertical.y, Right.y);
    Result.mRows[2] = _mm_set_ps(0.0f, Direction.z, Vertical.z, Right.z);
    Result.mRows[3] = _mm_set_ps(1.0f, -Direction.Dot(Position), -Vertical.Dot(Position), -Right.Dot(Position));

    return Result;
}

FMatrix FMatrix::Perspective(float FieldOfView, float Aspect, float NearPlane, float FarPlane) {
    const float Height{1.0f / std::tan(Math::ToRadians(FieldOfView) * 0.5f)};
    const float Width{Height / Aspect};
    const float Depth{FarPlane / (FarPlane - NearPlane)};

    FMatrix Result{};
    Result.mRows[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, Width);
    Result.mRows[1] = _mm_set_ps(0.0f, 0.0f, Height, 0.0f);
    Result.mRows[2] = _mm_set_ps(1.0f, Depth, 0.0f, 0.0f);
    Result.mRows[3] = _mm_set_ps(0.0f, -NearPlane * Depth, 0.0f, 0.0f);

    return Result;
}

FMatrix FMatrix::operator+(const FMatrix& Other) const {
    FMatrix Result{};

    for (Int32 Row{}; Row < 4; ++Row) {
        Result.mRows[Row] = _mm_add_ps(mRows[Row], Other.mRows[Row]);
    }

    return Result;
}

FMatrix FMatrix::operator-(const FMatrix& Other) const {
    FMatrix Result{};

    for (Int32 Row{}; Row < 4; ++Row) {
        Result.mRows[Row] = _mm_sub_ps(mRows[Row], Other.mRows[Row]);
    }

    return Result;
}

FMatrix FMatrix::operator*(const FMatrix& Other) const {
    const __m128 OtherX{Other.mRows[0]};
    const __m128 OtherY{Other.mRows[1]};
    const __m128 OtherZ{Other.mRows[2]};
    const __m128 OtherW{Other.mRows[3]};
    FMatrix Result{};

    for (Int32 Row{}; Row < 4; ++Row) {
        const __m128 Value{mRows[Row]};
        const __m128 X{_mm_mul_ps(Splat<0>(Value), OtherX)};
        const __m128 Y{_mm_mul_ps(Splat<1>(Value), OtherY)};
        const __m128 Z{_mm_mul_ps(Splat<2>(Value), OtherZ)};
        const __m128 W{_mm_mul_ps(Splat<3>(Value), OtherW)};

        Result.mRows[Row] = _mm_add_ps(_mm_add_ps(X, Y), _mm_add_ps(Z, W));
    }

    return Result;
}

FMatrix FMatrix::operator*(float Value) const {
    const __m128 Scale{_mm_set1_ps(Value)};
    FMatrix Result{};

    for (Int32 Row{}; Row < 4; ++Row) {
        Result.mRows[Row] = _mm_mul_ps(mRows[Row], Scale);
    }

    return Result;
}

FMatrix FMatrix::operator/(float Value) const {
    return *this * (1.0f / Value);
}

FMatrix& FMatrix::operator+=(const FMatrix& Other) {
    *this = *this + Other;

    return *this;
}

FMatrix& FMatrix::operator-=(const FMatrix& Other) {
    *this = *this - Other;

    return *this;
}

FMatrix& FMatrix::operator*=(const FMatrix& Other) {
    *this = *this * Other;

    return *this;
}

FMatrix& FMatrix::operator*=(float Value) {
    *this = *this * Value;

    return *this;
}

FMatrix& FMatrix::operator/=(float Value) {
    *this = *this / Value;

    return *this;
}

bool FMatrix::operator==(const FMatrix& Other) const {
    for (Int32 Row{}; Row < 4; ++Row) {
        if (!Equal<4>(mRows[Row], Other.mRows[Row])) {
            return false;
        }
    }

    return true;
}

FVector FMatrix::TransformPosition(const FVector& Position) const {
    const __m128 Homogeneous{_mm_add_ps(Position.mValue, _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f))};
    const __m128 Result{Transform(Homogeneous, mRows)};

    return FVector{_mm_div_ps(Result, Splat<3>(Result))};
}

FVector FMatrix::TransformVector(const FVector& Vector) const {
    return FVector{Transform(Vector.mValue, mRows)};
}

FVector FMatrix::GetAxis(Int32 Index) const {
    return FVector{mRows[Index]};
}

FRotator FMatrix::GetRotation() const {
    return GetQuaternion().Rotator();
}

FQuat FMatrix::GetQuaternion() const {
    FMatrix Rotation{*this};
    std::array<float, 3> Scales{};

    for (Int32 Row{}; Row < 3; ++Row) {
        const __m128 Axis{mRows[Row]};
        const __m128 SizeSquared{Math::Simd::Dot<3>(Axis, Axis)};

        if (!Math::Simd::IsFinite<3>(Axis) || _mm_cvtss_f32(SizeSquared) <= std::numeric_limits<float>::min()) {
            return {};
        }

        const __m128 Size{_mm_sqrt_ps(SizeSquared)};
        Scales[Row] = _mm_cvtss_f32(Size);
        Rotation.mRows[Row] = _mm_div_ps(Axis, Size);
    }

    const float Determinant{_mm_cvtss_f32(Math::Simd::Dot<3>(Cross3(Rotation.mRows[0], Rotation.mRows[1]), Rotation.mRows[2]))};

    if (!std::isfinite(Determinant) || std::abs(Determinant * Determinant - 1.0f) > 0.0001f) {
        return {};
    }

    if (Determinant < 0.0f) {
        const auto Largest{std::max_element(Scales.begin(), Scales.end())};
        const auto Row{static_cast<std::size_t>(Largest - Scales.begin())};
        Rotation.mRows[Row] = _mm_xor_ps(Rotation.mRows[Row], _mm_set1_ps(-0.0f));
    }

    const float Trace{Rotation.mRows[0].m128_f32[0] + Rotation.mRows[1].m128_f32[1] + Rotation.mRows[2].m128_f32[2]};
    FQuat Result{};

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

bool FMatrix::TryInverse(FMatrix& Result) const {
    FMatrix Working{*this};
    FMatrix Inverse{};
    double Determinant{1.0};

    if (!IsFinite()) {
        return false;
    }

    for (Int32 Column{}; Column < 4; ++Column) {
        Int32 Pivot{Column};

        for (Int32 Row{Column + 1}; Row < 4; ++Row) {
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

        for (Int32 Row{}; Row < 4; ++Row) {
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

FMatrix FMatrix::Transposed() const {
    FMatrix Result{*this};
    _MM_TRANSPOSE4_PS(Result.mRows[0], Result.mRows[1], Result.mRows[2], Result.mRows[3]);

    return Result;
}

float FMatrix::Determinant() const {
    constexpr std::array<std::array<Int32, 2>, 6> Columns{{{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}}};
    std::array<double, 6> Upper{};
    std::array<double, 6> Lower{};

    for (std::size_t Index{}; Index < Columns.size(); ++Index) {
        const Int32 A{Columns[Index][0]};
        const Int32 B{Columns[Index][1]};
        Upper[Index] = static_cast<double>(mRows[0].m128_f32[A]) * mRows[1].m128_f32[B] - static_cast<double>(mRows[0].m128_f32[B]) * mRows[1].m128_f32[A];
        Lower[Index] = static_cast<double>(mRows[2].m128_f32[A]) * mRows[3].m128_f32[B] - static_cast<double>(mRows[2].m128_f32[B]) * mRows[3].m128_f32[A];
    }

    return static_cast<float>(Upper[0] * Lower[5] - Upper[1] * Lower[4] + Upper[2] * Lower[3] + Upper[3] * Lower[2] - Upper[4] * Lower[1] + Upper[5] * Lower[0]);
}

FVector FMatrix::Origin() const {
    return FVector{mRows[3]};
}

bool FMatrix::IsFinite() const {
    for (Int32 Row{}; Row < 4; ++Row) {
        if (!Math::Simd::IsFinite<4>(mRows[Row])) {
            return false;
        }
    }

    return true;
}

bool FMatrix::IsIdentity(float Tolerance) const {
    return Equals(FMatrix{}, Tolerance);
}

bool FMatrix::Equals(const FMatrix& Other, float Tolerance) const {
    for (Int32 Row{}; Row < 4; ++Row) {
        if (!NearEqual<4>(mRows[Row], Other.mRows[Row], Tolerance)) {
            return false;
        }
    }

    return true;
}

FMatrix operator*(float Value, const FMatrix& Matrix) {
    return Matrix * Value;
}

float& FMatrix::operator()(Int32 Row, Int32 Column) {
    return mRows[Row].m128_f32[Column];
}

const float& FMatrix::operator()(Int32 Row, Int32 Column) const {
    return mRows[Row].m128_f32[Column];
}
