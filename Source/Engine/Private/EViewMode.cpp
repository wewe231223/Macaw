/**
 * @file EViewMode.cpp
 * @brief 장면 전체에 적용하는 메시 채우기 방식입니다.
 * @ingroup Engine
 */

#include "Engine/EViewMode.h"

namespace Engine {

    /**
     * @brief 뷰 모드가 GBuffer 속성 시각화에 해당하는지 확인합니다.
     * @param Mode 설정할 동작 또는 표시 모드입니다.
     * @return 버퍼 시각화 모드이면 true입니다.
     */
    bool IsBufferVisualization(Engine::EViewMode Mode) {
        return (Mode >= Engine::EViewMode::WorldNormal && Mode <= Engine::EViewMode::ReceiveShadows) || Mode == Engine::EViewMode::SceneStencil;
    }

}
