/**
 * @file FPropertyValue.cpp
 * @brief 프로퍼티가 지원하는 스칼라·수학 타입과 불투명 주소의 variant입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/FPropertyValue.h"

namespace CoreUObject {

    /**
     * @brief 프로퍼티 variant에 저장된 실제 값의 주소를 반환합니다.
     */
    void* GetPropertyData(CoreUObject::FPropertyValue& Value) {
        return std::visit([](auto& Typed) -> void* {
            return &Typed;
        }, Value);
    }

}
