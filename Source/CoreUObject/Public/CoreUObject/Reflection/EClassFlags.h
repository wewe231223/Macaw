/**
 * @file EClassFlags.h
 * @brief 리플렉션 클래스의 인스턴스 생성 정책을 지정하는 비트 플래그입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Types.h"

namespace CoreUObject {

    /**
     * @brief 리플렉션 클래스의 인스턴스 생성 정책을 지정하는 비트 플래그입니다.
     * @ingroup CoreUObject
     */
    enum class EClassFlags : Core::Uint32 {
        /**
         * @brief 추가 플래그가 없습니다.
         */
        None = 0,
        /**
         * @brief 추상 타입으로 분류하여 팩터리 생성을 금지합니다.
         */
        Abstract = 1,
        /**
         * @brief 기본 팩터리에 의한 인스턴스 생성을 금지합니다.
         */
        NoFactory = 2,
        /**
         * @brief 메타데이터 타입임을 표시합니다.
         */
        Metadata = 4
    };

    /**
     * @brief 두 플래그 집합의 비트 OR 결합을 반환합니다.
     * @param[in] Left 왼쪽 피연산자입니다.
     * @param[in] Right 오른쪽 피연산자입니다.
     * @return 두 플래그를 OR로 합친 값입니다.
     */
    CoreUObject::EClassFlags operator|(CoreUObject::EClassFlags Left, CoreUObject::EClassFlags Right);

}
