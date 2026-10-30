/**
 * @file SimdMath.h
 * @brief 수학 타입 내부의 SSE 레인, 벡터와 쿼터니언 연산을 제공합니다.
 * @ingroup Core
 */

#pragma once

#include <cstddef>
#include <emmintrin.h>
#include <limits>
#include <xmmintrin.h>

namespace Core {

    /**
     * @brief 수학 타입 구현에 사용하는 SIMD 내부 연산입니다.
     * @ingroup Core
     */
    namespace Math::Simd {
        /**
         * @brief 지정한 SIMD 성분을 네 레인 전체에 복제합니다.
         * @tparam Index 선택할 SIMD 레인 인덱스이며 0부터 3까지입니다.
         * @param[in] Value 연산할 SIMD 레지스터 값입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        template <std::size_t Index>
        __m128 Splat(__m128 Value);

        /**
         * @brief 지정한 SIMD 레인을 스칼라 float로 추출합니다.
         * @tparam Index 선택할 SIMD 레인 인덱스이며 0부터 3까지입니다.
         * @param[in] Value 연산할 SIMD 레지스터 값입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        template <std::size_t Index>
        float Extract(__m128 Value);

        /**
         * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
         * @tparam Components 연산할 유효 성분 수이며 2부터 4까지입니다.
         * @param[in] Left 왼쪽 피연산자입니다.
         * @param[in] Right 오른쪽 피연산자입니다.
         * @return 대응 성분 곱의 합입니다.
         */
        template <std::size_t Components>
        __m128 Dot(__m128 Left, __m128 Right);

        /**
         * @brief 사용하는 SIMD 성분이 모두 정확히 같은지 검사합니다.
         * @tparam Components 연산할 유효 성분 수이며 2부터 4까지입니다.
         * @param[in] Left 왼쪽 피연산자입니다.
         * @param[in] Right 오른쪽 피연산자입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        template <std::size_t Components>
        bool Equal(__m128 Left, __m128 Right);

        /**
         * @brief 사용하는 SIMD 성분의 차이가 모두 허용 오차 이내인지 검사합니다.
         * @tparam Components 연산할 유효 성분 수이며 2부터 4까지입니다.
         * @param[in] Left 왼쪽 피연산자입니다.
         * @param[in] Right 오른쪽 피연산자입니다.
         * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        template <std::size_t Components>
        bool NearEqual(__m128 Left, __m128 Right, float Tolerance);

        /**
         * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
         * @tparam Components 연산할 유효 성분 수이며 2부터 4까지입니다.
         * @param[in] Value 연산할 SIMD 레지스터 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        template <std::size_t Components>
        bool IsFinite(__m128 Value);

        /**
         * @brief 값을 제자리에서 정규화합니다.
         * @tparam Components 연산할 유효 성분 수이며 2부터 4까지입니다.
         * @param[in,out] Value 연산할 SIMD 레지스터 값입니다.
         * @return 정규화할 수 있으면 true이며, 퇴화하거나 유효하지 않은 값이면 false입니다.
         */
        template <std::size_t Components>
        bool Normalize(__m128& Value);

        /**
         * @brief 앞 세 SIMD 성분으로 Left × Right 외적을 계산합니다.
         * @param[in] Left 왼쪽 벡터의 x·y·z 성분입니다.
         * @param[in] Right 오른쪽 벡터의 x·y·z 성분입니다.
         * @return 외적을 x·y·z에 저장하고 w를 0으로 만든 SIMD 값입니다.
         */
        __m128 __vectorcall Cross3(__m128 Left, __m128 Right);
        /**
         * @brief x·y·z·w 순서의 두 쿼터니언에 Hamilton 곱을 적용합니다.
         * @param[in] Left 왼쪽 쿼터니언입니다.
         * @param[in] Right 오른쪽 쿼터니언입니다.
         * @return Left와 Right의 곱을 x·y·z·w에 저장한 쿼터니언입니다.
         */
        __m128 __vectorcall MultiplyQuaternion(__m128 Left, __m128 Right);
        /**
         * @brief 단위 쿼터니언으로 SIMD 벡터의 앞 세 성분을 회전합니다.
         * @param[in] Vector 회전할 벡터이며 w 성분은 유지합니다.
         * @param[in] Rotation x·y·z·w 순서의 정규화된 회전 쿼터니언입니다.
         * @return 회전된 벡터를 저장한 SIMD 값입니다.
         */
        __m128 __vectorcall RotateVector(__m128 Vector, __m128 Rotation);
        /**
         * @brief 동차 행 벡터에 네 SIMD 행으로 저장한 행렬을 곱합니다.
         * @param[in] Vector 변환할 x·y·z·w 동차 벡터입니다.
         * @param[in] Rows 변환 행렬의 SIMD 행 벡터 네 개입니다.
         * @return 각 행을 Vector의 대응 성분으로 가중 합산한 변환 결과입니다.
         */
        __m128 __vectorcall Transform(__m128 Vector, const __m128 (&Rows)[4]);
    }

    /**
     * @fn template <std::size_t Index> __m128 Core::Math::Simd::Splat(__m128 Value)
     * @brief 지정한 SIMD 성분을 네 레인 전체에 복제합니다.
     */
    template <std::size_t Index>
    __m128 Core::Math::Simd::Splat(__m128 Value) {
        static_assert(Index < 4);
        return _mm_shuffle_ps(Value, Value, _MM_SHUFFLE(Index, Index, Index, Index));
    }

    /**
     * @fn template <std::size_t Index> float Core::Math::Simd::Extract(__m128 Value)
     * @brief 지정한 SIMD 레인을 스칼라 float로 추출합니다.
     */
    template <std::size_t Index>
    float Core::Math::Simd::Extract(__m128 Value) {
        return _mm_cvtss_f32(Core::Math::Simd::Splat<Index>(Value));
    }

    /**
     * @fn template <std::size_t Components> __m128 Core::Math::Simd::Dot(__m128 Left, __m128 Right)
     * @brief 대응 성분의 곱을 합산하여 내적을 계산합니다.
     */
    template <std::size_t Components>
    __m128 Core::Math::Simd::Dot(__m128 Left, __m128 Right) {
        static_assert(Components >= 2 && Components <= 4);

        const __m128 Mask{_mm_castsi128_ps(_mm_set_epi32(Components == 4 ? -1 : 0, Components >= 3 ? -1 : 0, -1, -1))};
        const __m128 Product{_mm_and_ps(_mm_mul_ps(Left, Right), Mask)};
        const __m128 Pair{_mm_add_ps(Product, _mm_movehl_ps(Product, Product))};
        const __m128 Sum{_mm_add_ss(Pair, Core::Math::Simd::Splat<1>(Pair))};

        return Core::Math::Simd::Splat<0>(Sum);
    }

    /**
     * @fn template <std::size_t Components> bool Core::Math::Simd::Equal(__m128 Left, __m128 Right)
     * @brief 사용하는 SIMD 성분이 모두 정확히 같은지 검사합니다.
     */
    template <std::size_t Components>
    bool Core::Math::Simd::Equal(__m128 Left, __m128 Right) {
        static_assert(Components >= 2 && Components <= 4);

        constexpr int Mask{(1 << Components) - 1};
        return (_mm_movemask_ps(_mm_cmpeq_ps(Left, Right)) & Mask) == Mask;
    }

    /**
     * @fn template <std::size_t Components> bool Core::Math::Simd::NearEqual(__m128 Left, __m128 Right, float Tolerance)
     * @brief 사용하는 SIMD 성분의 차이가 모두 허용 오차 이내인지 검사합니다.
     */
    template <std::size_t Components>
    bool Core::Math::Simd::NearEqual(__m128 Left, __m128 Right, float Tolerance) {
        static_assert(Components >= 2 && Components <= 4);

        constexpr int Mask{(1 << Components) - 1};
        const __m128 Difference{_mm_andnot_ps(_mm_set1_ps(-0.0f), _mm_sub_ps(Left, Right))};

        return (_mm_movemask_ps(_mm_cmple_ps(Difference, _mm_set1_ps(Tolerance))) & Mask) == Mask;
    }

    /**
     * @fn template <std::size_t Components> bool Core::Math::Simd::IsFinite(__m128 Value)
     * @brief 모든 유효 성분에 NaN이나 무한대가 없는지 검사합니다.
     */
    template <std::size_t Components>
    bool Core::Math::Simd::IsFinite(__m128 Value) {
        static_assert(Components >= 2 && Components <= 4);

        constexpr int Mask{(1 << Components) - 1};
        const __m128 Magnitude{_mm_andnot_ps(_mm_set1_ps(-0.0f), Value)};

        return (_mm_movemask_ps(_mm_cmplt_ps(Magnitude, _mm_set1_ps(std::numeric_limits<float>::infinity()))) & Mask) == Mask;
    }

    /**
     * @fn template <std::size_t Components> bool Core::Math::Simd::Normalize(__m128& Value)
     * @brief 값을 제자리에서 정규화합니다.
     */
    template <std::size_t Components>
    bool Core::Math::Simd::Normalize(__m128& Value) {
        static_assert(Components >= 2 && Components <= 4);

        const __m128 Mask{_mm_castsi128_ps(_mm_set_epi32(Components == 4 ? -1 : 0, Components >= 3 ? -1 : 0, -1, -1))};
        Value = _mm_and_ps(Value, Mask);

        if (!Core::Math::Simd::IsFinite<Components>(Value)) {
            Value = _mm_setzero_ps();
            return false;
        }

        const __m128 Magnitude{_mm_andnot_ps(_mm_set1_ps(-0.0f), Value)};
        const __m128 Pair{_mm_max_ps(Magnitude, _mm_movehl_ps(Magnitude, Magnitude))};
        const __m128 Maximum{_mm_max_ss(Pair, Core::Math::Simd::Splat<1>(Pair))};
        const float Scale{_mm_cvtss_f32(Maximum)};

        if (Scale == 0.0f) {
            Value = _mm_setzero_ps();
            return false;
        }

        const __m128 Scaled{_mm_div_ps(Value, Core::Math::Simd::Splat<0>(Maximum))};
        const __m128 Size{_mm_sqrt_ps(Core::Math::Simd::Dot<Components>(Scaled, Scaled))};

        if (static_cast<double>(Scale) * _mm_cvtss_f32(Size) <= 0.000001) {
            Value = _mm_setzero_ps();
            return false;
        }

        Value = _mm_div_ps(Scaled, Size);

        return true;
    }

}
