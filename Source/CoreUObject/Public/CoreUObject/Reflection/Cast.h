/**
 * @file Cast.h
 * @brief 리플렉션 상속 관계를 검사하는 UObject 포인터 변환을 선언합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "CoreUObject/UObject.h"

#include <type_traits>

namespace CoreUObject {

    class UObject;

    /**
     * @brief null 및 리플렉션 타입을 검사한 뒤 요청한 UObject 파생 포인터로 변환합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @param[in] Object 대상 객체입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    template <typename T>
    T* Cast(CoreUObject::UObject* Object);

    /**
     * @brief null 및 리플렉션 타입을 검사한 뒤 요청한 UObject 파생 포인터로 변환합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @param[in] Object 대상 객체입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    template <typename T>
    const T* Cast(const CoreUObject::UObject* Object);

}

#include "CoreUObject/Reflection/CastTemplates.h"
