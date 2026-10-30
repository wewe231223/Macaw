/**
 * @file ShadowTests.h
 * @brief 그림자 아틀라스 할당과 광원별 그림자 렌더링을 검증합니다.
 */

#pragma once

class FRenderer;

/**
 * @brief 아틀라스 할당·캐스케이드 안정성과 광원 그림자 설정의 경계를 검증합니다.
 */
void TestShadowAllocation();
/**
 * @brief 광원별 그림자 생성과 그림자 표시 설정의 GPU 출력을 검증합니다.
 * @param[in] Renderer 검증에 사용할 렌더러입니다.
 */
void TestShadowRendering(FRenderer& Renderer);
