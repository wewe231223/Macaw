/**
 * @file SimdMath.cpp
 * @brief 수학 타입 내부의 SSE 레인, 벡터와 쿼터니언 연산을 제공합니다.
 * @ingroup Core
 */

#include "Math/SimdMath.h"

/**
 * @brief SIMD 레지스터의 앞 세 성분으로 3차원 외적을 계산합니다.
 * @param[in] Left 왼쪽 피연산자입니다.
 * @param[in] Right 오른쪽 피연산자입니다.
 * @return 연산을 적용한 새 값입니다.
 */
__m128 __vectorcall Math::Simd::Cross3(__m128 Left, __m128 Right) {
    const __m128 LeftYzx{_mm_shuffle_ps(Left, Left, _MM_SHUFFLE(3, 0, 2, 1))};
    const __m128 LeftZxy{_mm_shuffle_ps(Left, Left, _MM_SHUFFLE(3, 1, 0, 2))};
    const __m128 RightYzx{_mm_shuffle_ps(Right, Right, _MM_SHUFFLE(3, 0, 2, 1))};
    const __m128 RightZxy{_mm_shuffle_ps(Right, Right, _MM_SHUFFLE(3, 1, 0, 2))};

    return _mm_sub_ps(_mm_mul_ps(LeftYzx, RightZxy), _mm_mul_ps(LeftZxy, RightYzx));
}

/**
 * @brief SIMD 쿼터니언의 회전 합성 곱을 계산합니다.
 * @param[in] Left 왼쪽 피연산자입니다.
 * @param[in] Right 오른쪽 피연산자입니다.
 * @return 연산을 적용한 새 값입니다.
 */
__m128 __vectorcall Math::Simd::MultiplyQuaternion(__m128 Left, __m128 Right) {
    const __m128 Vector{_mm_add_ps(_mm_add_ps(_mm_mul_ps(Splat<3>(Left), Right), _mm_mul_ps(Splat<3>(Right), Left)), Cross3(Left, Right))};
    const __m128 Scalar{_mm_sub_ps(_mm_mul_ps(Splat<3>(Left), Splat<3>(Right)), Dot<3>(Left, Right))};
    const __m128 ScalarMask{_mm_castsi128_ps(_mm_set_epi32(-1, 0, 0, 0))};

    return _mm_or_ps(_mm_andnot_ps(ScalarMask, Vector), _mm_and_ps(ScalarMask, Scalar));
}

/**
 * @brief 현재 회전을 벡터에 적용합니다.
 * @param[in] Vector 연산에 사용할 벡터입니다.
 * @param[in] Rotation 적용할 오일러 또는 쿼터니언 회전입니다.
 * @return 연산을 적용한 새 값입니다.
 */
__m128 __vectorcall Math::Simd::RotateVector(__m128 Vector, __m128 Rotation) {
    const __m128 Cross{Cross3(Rotation, Vector)};
    const __m128 TwiceCross{_mm_add_ps(Cross, Cross)};

    return _mm_add_ps(Vector, _mm_add_ps(_mm_mul_ps(Splat<3>(Rotation), TwiceCross), Cross3(Rotation, TwiceCross)));
}

/**
 * @brief 동차 SIMD 벡터에 네 행으로 저장된 행렬을 적용합니다.
 * @param[in] Vector 연산에 사용할 벡터입니다.
 * @param[in] Rows 변환 행렬의 SIMD 행 벡터 네 개입니다.
 * @return 연산을 적용한 새 값입니다.
 */
__m128 __vectorcall Math::Simd::Transform(__m128 Vector, const __m128 (&Rows)[4]) {
    const __m128 X{_mm_mul_ps(Splat<0>(Vector), Rows[0])};
    const __m128 Y{_mm_mul_ps(Splat<1>(Vector), Rows[1])};
    const __m128 Z{_mm_mul_ps(Splat<2>(Vector), Rows[2])};
    const __m128 W{_mm_mul_ps(Splat<3>(Vector), Rows[3])};

    return _mm_add_ps(_mm_add_ps(X, Y), _mm_add_ps(Z, W));
}
