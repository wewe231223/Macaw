/**
 * @file FVectorRegister.h
 * @brief 대상 아키텍처에 따라 벡터 레지스터 선언을 선택합니다.
 * @note x64에서는 SSE 구현을 포함하고, 그 외 환경에는 VectorRegister 저장 구조체만 선언합니다.
 */

#pragma once

#if defined(__x86_64__) || defined(_M_X64)
// 인텔/AMD 환경
#include "Math/FVectorRegister_SSE.h"
//#elif defined(__aarch64__) || defined(_M_ARM64)
//// 애플 실리콘/모바일 환경 <-- 미구현
//#include <arm_neon.h>
//using VectorRegister = float32x4_t;
#else
/**
 * @brief SSE 구현을 선택하지 않은 환경의 16바이트 정렬 벡터 저장 구조체입니다.
 * @note 이 구조체 선언만으로 VectorSIMD 연산 구현이 제공되지는 않습니다.
 */
struct alignas(16) VectorRegister {
    /**
     * @brief 네 개의 단정밀도 성분을 저장하는 배열입니다.
     */
    float v[4];
};
#endif
