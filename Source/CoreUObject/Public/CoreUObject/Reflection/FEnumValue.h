/**
 * @file FEnumValue.h
 * @brief 열거값의 표시 이름과 Int32 저장 값을 묶습니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace CoreUObject {

    /**
     * @brief 열거값의 표시 이름과 Int32 저장 값을 묶습니다.
     * @ingroup CoreUObject
     */
    struct FEnumValue {
    public:
        /**
         * @brief 이름 또는 표시 문자열입니다.
         */
        Core::FString mName{};

        /**
         * @brief 열거값의 정수 값입니다.
         */
        Core::Int32 mValue{};
    };

}
