/**
 * @file SimdMath.cpp
 * @brief 수학 타입 내부의 SSE 레인, 벡터와 쿼터니언 연산을 제공합니다.
 * @ingroup Core
 */

#include "Math/SimdMath.h"

namespace Core {

    /**
     * @fn __m128 Core::Math::Simd::Cross3(__m128 Left, __m128 Right)
     * @brief SIMD 레지스터의 앞 세 성분으로 3차원 외적을 계산합니다.
     */
    __m128 __vectorcall Core::Math::Simd::Cross3(__m128 Left, __m128 Right) {
        const __m128 LeftYzx{_mm_shuffle_ps(Left, Left, _MM_SHUFFLE(3, 0, 2, 1))};
        const __m128 LeftZxy{_mm_shuffle_ps(Left, Left, _MM_SHUFFLE(3, 1, 0, 2))};
        const __m128 RightYzx{_mm_shuffle_ps(Right, Right, _MM_SHUFFLE(3, 0, 2, 1))};
        const __m128 RightZxy{_mm_shuffle_ps(Right, Right, _MM_SHUFFLE(3, 1, 0, 2))};

        return _mm_sub_ps(_mm_mul_ps(LeftYzx, RightZxy), _mm_mul_ps(LeftZxy, RightYzx));
    }

    /**
     * @fn __m128 Core::Math::Simd::MultiplyQuaternion(__m128 Left, __m128 Right)
     * @brief SIMD 쿼터니언의 회전 합성 곱을 계산합니다.
     */
    __m128 __vectorcall Core::Math::Simd::MultiplyQuaternion(__m128 Left, __m128 Right) {
        const __m128 Vector{_mm_add_ps(_mm_add_ps(_mm_mul_ps(Core::Math::Simd::Splat<3>(Left), Right), _mm_mul_ps(Core::Math::Simd::Splat<3>(Right), Left)), Core::Math::Simd::Cross3(Left, Right))};
        const __m128 Scalar{_mm_sub_ps(_mm_mul_ps(Core::Math::Simd::Splat<3>(Left), Core::Math::Simd::Splat<3>(Right)), Core::Math::Simd::Dot<3>(Left, Right))};
        const __m128 ScalarMask{_mm_castsi128_ps(_mm_set_epi32(-1, 0, 0, 0))};

        return _mm_or_ps(_mm_andnot_ps(ScalarMask, Vector), _mm_and_ps(ScalarMask, Scalar));
    }

    /**
     * @fn __m128 Core::Math::Simd::RotateVector(__m128 Vector, __m128 Rotation)
     * @brief 단위 쿼터니언으로 벡터의 앞 세 SIMD 성분을 회전합니다.
     */
    __m128 __vectorcall Core::Math::Simd::RotateVector(__m128 Vector, __m128 Rotation) {
        const __m128 Cross{Core::Math::Simd::Cross3(Rotation, Vector)};
        const __m128 TwiceCross{_mm_add_ps(Cross, Cross)};

        return _mm_add_ps(Vector, _mm_add_ps(_mm_mul_ps(Core::Math::Simd::Splat<3>(Rotation), TwiceCross), Core::Math::Simd::Cross3(Rotation, TwiceCross)));
    }

    /**
     * @fn __m128 Core::Math::Simd::Transform(__m128 Vector, const __m128 (&Rows)[4])
     * @brief 동차 SIMD 벡터에 네 행으로 저장된 행렬을 적용합니다.
     */
    __m128 __vectorcall Core::Math::Simd::Transform(__m128 Vector, const __m128 (&Rows)[4]) {
        const __m128 X{_mm_mul_ps(Core::Math::Simd::Splat<0>(Vector), Rows[0])};
        const __m128 Y{_mm_mul_ps(Core::Math::Simd::Splat<1>(Vector), Rows[1])};
        const __m128 Z{_mm_mul_ps(Core::Math::Simd::Splat<2>(Vector), Rows[2])};
        const __m128 W{_mm_mul_ps(Core::Math::Simd::Splat<3>(Vector), Rows[3])};

        return _mm_add_ps(_mm_add_ps(X, Y), _mm_add_ps(Z, W));
    }

}
