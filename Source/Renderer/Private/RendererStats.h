/**
 * @file RendererStats.h
 * @brief 렌더링과 그림자 패스의 드로·바인딩·메모리 통계 항목입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Stats.h"

namespace RendererStats {
    /**
     * @brief 제출한 메시 삼각형 수를 측정하는 통계 항목입니다.
     */
    extern FStat Triangles;
    /**
     * @brief 파이프라인 바인딩 횟수를 측정하는 통계 항목입니다.
     */
    extern FStat PipelineBinds;
    /**
     * @brief 머티리얼 바인딩 횟수를 측정하는 통계 항목입니다.
     */
    extern FStat MaterialBinds;
    /**
     * @brief 메시 버퍼 바인딩 횟수를 측정하는 통계 항목입니다.
     */
    extern FStat MeshBinds;
    /**
     * @brief 그림자 맵 면·캐스케이드 수를 측정하는 통계 항목입니다.
     */
    extern FStat ShadowMaps;
    /**
     * @brief 그림자 깊이 패스의 드로 호출 수를 측정하는 통계 항목입니다.
     */
    extern FStat ShadowDrawCalls;
    /**
     * @brief 그림자 해상도를 줄인 광원 수를 측정하는 통계 항목입니다.
     */
    extern FStat ShadowReducedLights;
    /**
     * @brief 그림자 할당을 생략한 광원 수를 측정하는 통계 항목입니다.
     */
    extern FStat ShadowDroppedLights;
    /**
     * @brief 그림자 아틀라스의 전체 바이트 수를 측정하는 통계 항목입니다.
     */
    extern FStat ShadowAtlasBytes;
    /**
     * @brief 그림자 타일의 할당 바이트 수를 측정하는 통계 항목입니다.
     */
    extern FStat ShadowAllocatedBytes;
    /**
     * @brief 모든 뷰에 제출한 방향광 수를 측정하는 통계 항목입니다.
     */
    extern FStat DirectionalLights;
    /**
     * @brief 모든 뷰에 제출한 점광원 수를 측정하는 통계 항목입니다.
     */
    extern FStat PointLights;
    /**
     * @brief 모든 뷰에 제출한 스포트라이트 수를 측정하는 통계 항목입니다.
     */
    extern FStat SpotLights;
}
