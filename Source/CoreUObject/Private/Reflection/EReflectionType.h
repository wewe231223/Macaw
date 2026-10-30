/**
 * @file EReflectionType.h
 * @brief 레지스트리 정의가 클래스, 구조체 또는 열거형인지 구분합니다.
 * @ingroup CoreUObject
 */

#pragma once

namespace CoreUObject {

    /**
     * @brief 레지스트리 정의가 클래스, 구조체 또는 열거형인지 구분합니다.
     * @ingroup CoreUObject
     */
    enum class EReflectionType {
        /**
         * @brief UObject 클래스 메타데이터입니다.
         */
        Class,
        /**
         * @brief 일반 구조체 메타데이터입니다.
         */
        Struct,
        /**
         * @brief 열거형 메타데이터 또는 Int32로 저장하는 열거값입니다.
         */
        Enum
    };

}
