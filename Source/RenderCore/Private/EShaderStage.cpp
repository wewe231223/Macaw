/**
 * @file EShaderStage.cpp
 * @brief 셰이더 단계 플래그를 결합하고 검사합니다.
 * @ingroup RenderCore
 */

#include "RenderCore/EShaderStage.h"

namespace RenderCore {

    /**
     * @brief 셰이더 단계 플래그를 비트 OR로 결합합니다.
     */
    RenderCore::EShaderStage operator|(RenderCore::EShaderStage Left, RenderCore::EShaderStage Right) {
        return static_cast<RenderCore::EShaderStage>(static_cast<Core::Uint8>(Left) | static_cast<Core::Uint8>(Right));
    }

    /**
     * @brief 셰이더 단계 플래그에서 공통으로 설정된 비트를 구합니다.
     */
    RenderCore::EShaderStage operator&(RenderCore::EShaderStage Left, RenderCore::EShaderStage Right) {
        return static_cast<RenderCore::EShaderStage>(static_cast<Core::Uint8>(Left) & static_cast<Core::Uint8>(Right));
    }

    /**
     * @brief 플래그에 특정 셰이더 단계가 포함되는지 확인합니다.
     */
    bool HasStage(RenderCore::EShaderStage Stages, RenderCore::EShaderStage Stage) {
        return (Stages & Stage) != RenderCore::EShaderStage::None;
    }

}
