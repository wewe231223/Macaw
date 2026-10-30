/**
 * @file EPropertyFlags.h
 * @brief 프로퍼티의 편집, 저장, 읽기 전용 및 일시 상태를 지정하는 비트 플래그입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Types.h"

namespace Core {

    /**
     * @brief 프로퍼티의 편집, 저장, 읽기 전용 및 일시 상태를 지정하는 비트 플래그입니다.
     * @ingroup Core
     */
    enum class EPropertyFlags : Core::Uint32 {
        /**
         * @brief 추가 플래그가 없습니다.
         */
        None = 0,
        /**
         * @brief 에디터에서 프로퍼티 편집을 허용합니다.
         */
        Edit = 1,
        /**
         * @brief 프로퍼티를 장면 직렬화에 포함합니다.
         */
        Serialize = 2,
        /**
         * @brief 일반 편집 경로의 쓰기를 금지합니다.
         */
        ReadOnly = 4,
        /**
         * @brief 런타임 전용이므로 직렬화에서 제외합니다.
         */
        Transient = 8
    };

    /**
     * @brief 두 플래그 집합의 비트 OR 결합을 반환합니다.
     * @param[in] Left 왼쪽 피연산자입니다.
     * @param[in] Right 오른쪽 피연산자입니다.
     * @return 두 플래그를 OR로 합친 값입니다.
     */
    Core::EPropertyFlags operator|(Core::EPropertyFlags Left, Core::EPropertyFlags Right);

}
