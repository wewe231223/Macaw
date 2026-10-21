#include "Math/FVectorRegister.h"
#include <cmath>

#if defined(__x86_64__) || defined(_M_X64)

FVectorRegister VectorSIMD::LoadFloat3(const float* Ptr) {
    return _mm_set_ps(0.0f, Ptr[2], Ptr[1], Ptr[0]);
}

void VectorSIMD::StoreFloat3(float* Ptr, FVectorRegister V) {
    alignas(16) float Temp[4]{};

    _mm_store_ps(Temp, V);
    Ptr[0] = Temp[0];
    Ptr[1] = Temp[1];
    Ptr[2] = Temp[2];
}

FVectorRegister VectorSIMD::LoadAligned(const float* Ptr) {
    return _mm_load_ps(Ptr);
}

void VectorSIMD::StoreAligned(float* Ptr, FVectorRegister V) {
    _mm_store_ps(Ptr, V);
}

FVectorRegister VectorSIMD::SetWZero(FVectorRegister A) {
    return _mm_and_ps(A, _mm_castsi128_ps(_mm_set_epi32(0, -1, -1, -1)));
}

FVectorRegister VectorSIMD::Rsqrt(FVectorRegister V) {
    return _mm_rsqrt_ps(V);
}

FVectorRegister VectorSIMD::Reciprocal(FVectorRegister A) {
    return _mm_div_ps(VectorSIMD::SetVal(1.0f), A);
}

FVectorRegister VectorSIMD::Min(FVectorRegister A, FVectorRegister B) {
    return _mm_min_ps(A, B);
}

FVectorRegister VectorSIMD::Max(FVectorRegister A, FVectorRegister B) {
    return _mm_max_ps(A, B);
}

FVectorRegister VectorSIMD::Abs(FVectorRegister A) {
    const FVectorRegister Mask{_mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF))};

    return _mm_and_ps(A, Mask);
}

bool VectorSIMD::IsNearlyZero(FVectorRegister A, float Tolerance) {
    const FVectorRegister Absolute{VectorSIMD::Abs(A)};
    const FVectorRegister Limit{_mm_set1_ps(Tolerance)};
    const FVectorRegister Comparison{_mm_cmple_ps(Absolute, Limit)};

    return _mm_movemask_ps(Comparison) == 0xF;
}

FVectorRegister VectorSIMD::Dot4(FVectorRegister A, FVectorRegister B) {
    return _mm_dp_ps(A, B, 0xFF);
}

FVectorRegister VectorSIMD::And(FVectorRegister A, FVectorRegister B) {
    return _mm_and_ps(A, B);
}

FVectorRegister VectorSIMD::Or(FVectorRegister A, FVectorRegister B) {
    return _mm_or_ps(A, B);
}

FVectorRegister VectorSIMD::Xor(FVectorRegister A, FVectorRegister B) {
    return _mm_xor_ps(A, B);
}

FVectorRegister VectorSIMD::AndNot(FVectorRegister A, FVectorRegister B) {
    return _mm_andnot_ps(A, B);
}

FVectorRegister VectorSIMD::MultiplyAdd(FVectorRegister A, FVectorRegister B, FVectorRegister C) {
    return VectorSIMD::Add(VectorSIMD::Mul(A, B), C);
}

FVectorRegister VectorSIMD::NegativeMultiplyAdd(FVectorRegister A, FVectorRegister B, FVectorRegister C) {
    return VectorSIMD::Sub(C, VectorSIMD::Mul(A, B));
}

float VectorSIMD::HorizontalAdd4(FVectorRegister V) {
    FVectorRegister Shuffled{_mm_shuffle_ps(V, V, _MM_SHUFFLE(2, 3, 0, 1))};

    V = _mm_add_ps(V, Shuffled);
    Shuffled = _mm_shuffle_ps(V, V, _MM_SHUFFLE(1, 0, 3, 2));
    V = _mm_add_ps(V, Shuffled);

    return _mm_cvtss_f32(V);
}

FVectorRegister VectorSIMD::Normalized(FVectorRegister A) {
    const float SquaredLength{VectorSIMD::LengthSquared(A)};

    if (!std::isfinite(SquaredLength) || SquaredLength <= 0.0f) {
        return VectorSIMD::SetZero();
    }

    return VectorSIMD::Div(A, VectorSIMD::SetVal(std::sqrt(SquaredLength)));
}

#endif
