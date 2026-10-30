/**
 * @file Unlit.hlsl
 * @brief 조명 계산 없이 재질 색상과 텍스처를 표시하는 메시 셰이더입니다.
 */

/**
 * @brief 공통 메시 셰이더에서 광원 계산을 생략하고 재질 색상만 출력합니다.
 */
#define MACAW_UNLIT 1
#include "Lit.hlsl"
