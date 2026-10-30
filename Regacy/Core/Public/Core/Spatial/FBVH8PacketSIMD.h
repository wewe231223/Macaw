/**
 * @file FBVH8PacketSIMD.h
 * @brief 삼각형 패킷 검사에 사용할 SSE 4레인 및 AVX 8레인 연산 인터페이스를 정의한다.
 * @details FSIMD8은 __AVX__가 정의된 컴파일에서 제공된다.
 */

#pragma once

#include "Core/Common.h"

#include <immintrin.h>

namespace BVH8 {
    /**
     * @brief 네 float 레인을 동시에 계산하는 SSE 연산 구현.
     */
    struct FSIMD4 {
        /**
         * @brief 네 float 레인과 비교 마스크를 담는 128비트 SIMD 타입.
         */
        using V = __m128;

        /**
         * @brief 한 번에 처리하는 float 레인 수. SSE 구현은 4이다.
         */
        static constexpr Uint32 Width = 4;

        /**
         * @brief 모든 레인을 같은 값으로 채운다.
         * @param X 각 레인에 사용할 값.
         * @return X를 모든 레인에 복제한 벡터.
         */
        static __forceinline V Set(float X) {
            return _mm_set1_ps(X);
        }

        /**
         * @brief 정렬된 연속 float 네 개를 읽는다.
         * @param P 16바이트로 정렬되고 네 float를 읽을 수 있는 주소.
         * @return 읽은 네 레인의 벡터.
         */
        static __forceinline V Load(const float* P) {
            return _mm_load_ps(P);
        }

        /**
         * @brief 레인별로 두 벡터를 더한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A + B.
         */
        static __forceinline V Add(V A, V B) {
            return _mm_add_ps(A, B);
        }

        /**
         * @brief 레인별로 첫 벡터에서 두 번째 벡터를 뺀다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A - B.
         */
        static __forceinline V Sub(V A, V B) {
            return _mm_sub_ps(A, B);
        }

        /**
         * @brief 레인별로 두 벡터를 곱한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A * B.
         */
        static __forceinline V Mul(V A, V B) {
            return _mm_mul_ps(A, B);
        }

        /**
         * @brief 레인별로 첫 벡터를 두 번째 벡터로 나눈다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A / B.
         */
        static __forceinline V Div(V A, V B) {
            return _mm_div_ps(A, B);
        }

        /**
         * @brief 두 벡터의 비트 단위 배타적 논리합을 계산한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return 각 레인의 비트 단위 A XOR B.
         */
        static __forceinline V Xor(V A, V B) {
            return _mm_xor_ps(A, B);
        }

        /**
         * @brief 두 벡터의 비트 단위 논리곱을 계산한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return 각 레인의 비트 단위 A AND B.
         */
        static __forceinline V And(V A, V B) {
            return _mm_and_ps(A, B);
        }

        /**
         * @brief 레인별로 첫 값이 두 번째 값 이상인지 비교한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A >= B인 레인은 모든 비트 1, 나머지는 모든 비트 0인 마스크.
         */
        static __forceinline V GE(V A, V B) {
            return _mm_cmpge_ps(A, B);
        }

        /**
         * @brief 레인별로 첫 값이 두 번째 값 이하인지 비교한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A <= B인 레인은 모든 비트 1, 나머지는 모든 비트 0인 마스크.
         */
        static __forceinline V LE(V A, V B) {
            return _mm_cmple_ps(A, B);
        }

        /**
         * @brief 레인별 비교 마스크에 따라 두 벡터의 값을 선택한다.
         * @param Mask 각 레인이 모든 비트 1 또는 모든 비트 0인 비교 마스크.
         * @param Yes 참인 레인에서 선택할 값.
         * @param No 거짓인 레인에서 선택할 값.
         * @return 마스크에 따라 선택한 레인 벡터.
         */
        static __forceinline V Select(V Mask, V Yes, V No) {
            return _mm_or_ps(_mm_and_ps(Mask, Yes), _mm_andnot_ps(Mask, No));
        }

        /**
         * @brief 각 레인의 부호 비트를 정수 비트 마스크로 추출한다.
         * @param A 비교 결과 또는 부호 비트를 검사할 벡터.
         * @return 레인 순서대로 부호 비트를 배치한 정수.
         */
        static __forceinline Uint32 Bits(V A) {
            return static_cast<Uint32>(_mm_movemask_ps(A));
        }

        /**
         * @brief 정수의 하위 비트들을 SIMD 비교 마스크로 확장한다.
         * @param M 레인별 활성 여부를 나타내는 비트 마스크.
         * @return 각 활성 레인은 모든 비트 1, 나머지는 모든 비트 0인 벡터.
         */
        static __forceinline V Mask(Uint32 M) {
            return _mm_castsi128_ps(_mm_setr_epi32(-(int)(M & 1), -(int)((M >> 1) & 1), -(int)((M >> 2) & 1), -(int)((M >> 3) & 1)));
        }

        /**
         * @brief float 레인들의 최솟값을 하나의 스칼라로 축약한다.
         * @param A 최솟값을 구할 벡터.
         * @return SIMD 최소 연산으로 축약한 값.
         */
        static __forceinline float MinLane(V A) {
            A = _mm_min_ps(A, _mm_movehl_ps(A, A));

            return _mm_cvtss_f32(_mm_min_ss(A, _mm_shuffle_ps(A, A, 1)));
        }
    };

#if defined(__AVX__)
    /**
     * @brief 여덟 float 레인을 동시에 계산하는 AVX 연산 구현.
     * @pre 이 구현을 실행할 CPU와 운영체제가 AVX를 지원해야 한다.
     */
    struct FSIMD8 {
        /**
         * @brief 여덟 float 레인과 비교 마스크를 담는 256비트 SIMD 타입.
         */
        using V = __m256;

        /**
         * @brief 한 번에 처리하는 float 레인 수. AVX 구현은 8이다.
         */
        static constexpr Uint32 Width = 8;

        /**
         * @brief 모든 레인을 같은 값으로 채운다.
         * @param X 각 레인에 사용할 값.
         * @return X를 모든 레인에 복제한 벡터.
         */
        static __forceinline V Set(float X) {
            return _mm256_set1_ps(X);
        }

        /**
         * @brief 정렬된 연속 float 여덟 개를 읽는다.
         * @param P 32바이트로 정렬되고 여덟 float를 읽을 수 있는 주소.
         * @return 읽은 여덟 레인의 벡터.
         */
        static __forceinline V Load(const float* P) {
            return _mm256_load_ps(P);
        }

        /**
         * @brief 레인별로 두 벡터를 더한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A + B.
         */
        static __forceinline V Add(V A, V B) {
            return _mm256_add_ps(A, B);
        }

        /**
         * @brief 레인별로 첫 벡터에서 두 번째 벡터를 뺀다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A - B.
         */
        static __forceinline V Sub(V A, V B) {
            return _mm256_sub_ps(A, B);
        }

        /**
         * @brief 레인별로 두 벡터를 곱한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A * B.
         */
        static __forceinline V Mul(V A, V B) {
            return _mm256_mul_ps(A, B);
        }

        /**
         * @brief 레인별로 첫 벡터를 두 번째 벡터로 나눈다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A / B.
         */
        static __forceinline V Div(V A, V B) {
            return _mm256_div_ps(A, B);
        }

        /**
         * @brief 두 벡터의 비트 단위 배타적 논리합을 계산한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return 각 레인의 비트 단위 A XOR B.
         */
        static __forceinline V Xor(V A, V B) {
            return _mm256_xor_ps(A, B);
        }

        /**
         * @brief 두 벡터의 비트 단위 논리곱을 계산한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return 각 레인의 비트 단위 A AND B.
         */
        static __forceinline V And(V A, V B) {
            return _mm256_and_ps(A, B);
        }

        /**
         * @brief 레인별로 첫 값이 두 번째 값 이상인지 비교한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A >= B인 레인은 모든 비트 1, 나머지는 모든 비트 0인 마스크.
         */
        static __forceinline V GE(V A, V B) {
            return _mm256_cmp_ps(A, B, _CMP_GE_OQ);
        }

        /**
         * @brief 레인별로 첫 값이 두 번째 값 이하인지 비교한다.
         * @param A 첫 번째 벡터.
         * @param B 두 번째 벡터.
         * @return A <= B인 레인은 모든 비트 1, 나머지는 모든 비트 0인 마스크.
         */
        static __forceinline V LE(V A, V B) {
            return _mm256_cmp_ps(A, B, _CMP_LE_OQ);
        }

        /**
         * @brief 레인별 비교 마스크에 따라 두 벡터의 값을 선택한다.
         * @param Mask 각 레인이 모든 비트 1 또는 모든 비트 0인 비교 마스크.
         * @param Yes 참인 레인에서 선택할 값.
         * @param No 거짓인 레인에서 선택할 값.
         * @return 마스크에 따라 선택한 레인 벡터.
         */
        static __forceinline V Select(V Mask, V Yes, V No) {
            return _mm256_blendv_ps(No, Yes, Mask);
        }

        /**
         * @brief 각 레인의 부호 비트를 정수 비트 마스크로 추출한다.
         * @param A 비교 결과 또는 부호 비트를 검사할 벡터.
         * @return 레인 순서대로 부호 비트를 배치한 정수.
         */
        static __forceinline Uint32 Bits(V A) {
            return static_cast<Uint32>(_mm256_movemask_ps(A));
        }

        /**
         * @brief 정수의 하위 비트들을 SIMD 비교 마스크로 확장한다.
         * @param M 레인별 활성 여부를 나타내는 비트 마스크.
         * @return 각 활성 레인은 모든 비트 1, 나머지는 모든 비트 0인 벡터.
         */
        static __forceinline V Mask(Uint32 M) {
            return _mm256_castsi256_ps(_mm256_setr_epi32(-(int)(M & 1), -(int)((M >> 1) & 1), -(int)((M >> 2) & 1), -(int)((M >> 3) & 1), -(int)((M >> 4) & 1), -(int)((M >> 5) & 1), -(int)((M >> 6) & 1), -(int)((M >> 7) & 1)));
        }

        /**
         * @brief float 레인들의 최솟값을 하나의 스칼라로 축약한다.
         * @param A 최솟값을 구할 벡터.
         * @return SIMD 최소 연산으로 축약한 값.
         */
        static __forceinline float MinLane(V A) {
            return FSIMD4::MinLane(_mm_min_ps(_mm256_castps256_ps128(A), _mm256_extractf128_ps(A, 1)));
        }
    };
#endif
}
