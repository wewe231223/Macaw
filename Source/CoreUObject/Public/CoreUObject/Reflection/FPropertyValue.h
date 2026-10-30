/**
 * @file FPropertyValue.h
 * @brief 프로퍼티가 지원하는 스칼라·수학 타입과 불투명 주소의 variant입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Guid.h"
#include "Core/Math.h"

#include <variant>

namespace CoreUObject {

    /**
     * @brief 프로퍼티가 지원하는 스칼라·수학 타입과 불투명 주소의 variant입니다.
     */
    using FPropertyValue = std::variant<bool, Core::Int32, Core::Uint32, Core::Uint64, float, Core::FString, Core::FGuid, Core::FVector2, Core::FVector, Core::FVector4, Core::FQuat, Core::FRotator, Core::FMatrix, Core::FTransform, Core::FBox, Core::FRay, const void*>;

    /**
     * @brief 프로퍼티 variant에 저장된 실제 값의 주소를 반환합니다.
     * @param[in,out] Value 프로퍼티 타입에 대응하는 variant 값입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    void* GetPropertyData(CoreUObject::FPropertyValue& Value);

}
