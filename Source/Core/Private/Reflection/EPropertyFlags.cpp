/**
 * @file EPropertyFlags.cpp
 * @brief 프로퍼티의 편집, 저장, 읽기 전용 및 일시 상태를 지정하는 비트 플래그입니다.
 * @ingroup Core
 */

#include "Core/Reflection/EPropertyFlags.h"

namespace Core {

    /**
     * @brief 두 플래그 집합의 비트 OR 결합을 반환합니다.
     */
    Core::EPropertyFlags operator|(Core::EPropertyFlags Left, Core::EPropertyFlags Right) {
        return static_cast<Core::EPropertyFlags>(static_cast<Core::Uint32>(Left) | static_cast<Core::Uint32>(Right));
    }

}
