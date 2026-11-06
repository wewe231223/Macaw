/**
 * @file USpriteComponent.cpp
 * @brief 로컬 YZ 평면의 고정 텍스처 스프라이트를 제공합니다.
 * @ingroup Engine
 */

#include "Engine/Component/USpriteComponent.h"

namespace Engine {

    /**
     * @brief 카메라 정렬을 끈 고정 평면 스프라이트를 생성합니다.
     */
    Engine::USpriteComponent::USpriteComponent() {
        SetScreenAligned(false);
    }

    Engine::USpriteComponent::~USpriteComponent() = default;

}
