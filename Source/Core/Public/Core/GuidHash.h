/**
 * @file GuidHash.h
 * @brief GUID의 해시 컨테이너 연동을 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"

namespace Core {

    /**
     * @brief FGuid를 해시 컨테이너의 키로 사용하기 위한 해시 함수 객체입니다.
     * @ingroup Core
     */
    struct FGuidHash {
    public:
        /**
         * @brief GUID의 해시를 계산합니다.
         * @param[in] Value 입력 값입니다.
         * @return GUID 바이트로 계산한 해시 값입니다.
         */
        std::size_t operator()(const Core::FGuid& Value) const noexcept;
    };

}
