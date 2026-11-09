/**
 * @file EShaderStage.h
 * @brief 자원을 바인딩할 셰이더 단계 플래그와 비트 연산을 선언합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"

namespace RenderCore {

    /**
     * @brief 자원을 바인딩할 셰이더 단계를 비트 플래그로 나타냅니다.
     */
    enum class EShaderStage : Core::Uint8 {
        /**
         * @brief 셰이더 단계를 지정하지 않은 상태입니다.
         */
        None = 0,
        /**
         * @brief 정점 셰이더 단계입니다.
         */
        Vertex = 1,
        /**
         * @brief 픽셀 셰이더 단계입니다.
         */
        Pixel = 2,
        /**
         * @brief 컴퓨트 셰이더 단계입니다.
         */
        Compute = 4
    };

    /**
     * @brief 셰이더 단계 플래그를 비트 OR로 결합합니다.
     * @param[in] Left 결합할 왼쪽 값입니다.
     * @param[in] Right 결합할 오른쪽 값입니다.
     * @return 비트 연산을 적용한 셰이더 단계 플래그입니다.
     */
    RenderCore::EShaderStage operator|(RenderCore::EShaderStage Left, RenderCore::EShaderStage Right);

    /**
     * @brief 셰이더 단계 플래그에서 공통으로 설정된 비트를 구합니다.
     * @param[in] Left 비교할 왼쪽 값입니다.
     * @param[in] Right 비교할 오른쪽 값입니다.
     * @return 비트 연산을 적용한 셰이더 단계 플래그입니다.
     */
    RenderCore::EShaderStage operator&(RenderCore::EShaderStage Left, RenderCore::EShaderStage Right);

    /**
     * @brief 플래그에 특정 셰이더 단계가 포함되는지 확인합니다.
     * @param[in] Stages 검사할 셰이더 단계 플래그입니다.
     * @param[in] Stage 찾을 셰이더 단계입니다.
     * @return 포함되면 true입니다.
     */
    bool HasStage(RenderCore::EShaderStage Stages, RenderCore::EShaderStage Stage);

}
