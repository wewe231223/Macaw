#pragma once

#include <cstddef>
#include <emmintrin.h>
#include <limits>
#include <xmmintrin.h>

namespace Math::Simd {
    template <std::size_t Index>
    __m128 Splat(__m128 Value);

    template <std::size_t Index>
    float Extract(__m128 Value);

    template <std::size_t Components>
    __m128 Dot(__m128 Left, __m128 Right);

    template <std::size_t Components>
    bool Equal(__m128 Left, __m128 Right);

    template <std::size_t Components>
    bool NearEqual(__m128 Left, __m128 Right, float Tolerance);

    template <std::size_t Components>
    bool IsFinite(__m128 Value);

    template <std::size_t Components>
    bool Normalize(__m128& Value);

    __m128 __vectorcall Cross3(__m128 Left, __m128 Right);
    __m128 __vectorcall MultiplyQuaternion(__m128 Left, __m128 Right);
    __m128 __vectorcall RotateVector(__m128 Vector, __m128 Rotation);
    __m128 __vectorcall Transform(__m128 Vector, const __m128 (&Rows)[4]);
}

template <std::size_t Index>
__m128 Math::Simd::Splat(__m128 Value) {
    static_assert(Index < 4);
    return _mm_shuffle_ps(Value, Value, _MM_SHUFFLE(Index, Index, Index, Index));
}

template <std::size_t Index>
float Math::Simd::Extract(__m128 Value) {
    return _mm_cvtss_f32(Splat<Index>(Value));
}

template <std::size_t Components>
__m128 Math::Simd::Dot(__m128 Left, __m128 Right) {
    static_assert(Components >= 2 && Components <= 4);
    const __m128 Mask{_mm_castsi128_ps(_mm_set_epi32(Components == 4 ? -1 : 0, Components >= 3 ? -1 : 0, -1, -1))};
    const __m128 Product{_mm_and_ps(_mm_mul_ps(Left, Right), Mask)};
    const __m128 Pair{_mm_add_ps(Product, _mm_movehl_ps(Product, Product))};
    const __m128 Sum{_mm_add_ss(Pair, Splat<1>(Pair))};

    return Splat<0>(Sum);
}

template <std::size_t Components>
bool Math::Simd::Equal(__m128 Left, __m128 Right) {
    static_assert(Components >= 2 && Components <= 4);
    constexpr int Mask{(1 << Components) - 1};
    return (_mm_movemask_ps(_mm_cmpeq_ps(Left, Right)) & Mask) == Mask;
}

template <std::size_t Components>
bool Math::Simd::NearEqual(__m128 Left, __m128 Right, float Tolerance) {
    static_assert(Components >= 2 && Components <= 4);
    constexpr int Mask{(1 << Components) - 1};
    const __m128 Difference{_mm_andnot_ps(_mm_set1_ps(-0.0f), _mm_sub_ps(Left, Right))};

    return (_mm_movemask_ps(_mm_cmple_ps(Difference, _mm_set1_ps(Tolerance))) & Mask) == Mask;
}

template <std::size_t Components>
bool Math::Simd::IsFinite(__m128 Value) {
    static_assert(Components >= 2 && Components <= 4);
    constexpr int Mask{(1 << Components) - 1};
    const __m128 Magnitude{_mm_andnot_ps(_mm_set1_ps(-0.0f), Value)};

    return (_mm_movemask_ps(_mm_cmplt_ps(Magnitude, _mm_set1_ps(std::numeric_limits<float>::infinity()))) & Mask) == Mask;
}

template <std::size_t Components>
bool Math::Simd::Normalize(__m128& Value) {
    static_assert(Components >= 2 && Components <= 4);
    const __m128 Mask{_mm_castsi128_ps(_mm_set_epi32(Components == 4 ? -1 : 0, Components >= 3 ? -1 : 0, -1, -1))};
    Value = _mm_and_ps(Value, Mask);

    if (!IsFinite<Components>(Value)) {
        Value = _mm_setzero_ps();
        return false;
    }

    const __m128 Magnitude{_mm_andnot_ps(_mm_set1_ps(-0.0f), Value)};
    const __m128 Pair{_mm_max_ps(Magnitude, _mm_movehl_ps(Magnitude, Magnitude))};
    const __m128 Maximum{_mm_max_ss(Pair, Splat<1>(Pair))};
    const float Scale{_mm_cvtss_f32(Maximum)};

    if (Scale == 0.0f) {
        Value = _mm_setzero_ps();
        return false;
    }

    const __m128 Scaled{_mm_div_ps(Value, Splat<0>(Maximum))};
    const __m128 Size{_mm_sqrt_ps(Dot<Components>(Scaled, Scaled))};

    if (static_cast<double>(Scale) * _mm_cvtss_f32(Size) <= 0.000001) {
        Value = _mm_setzero_ps();
        return false;
    }

    Value = _mm_div_ps(Scaled, Size);

    return true;
}
