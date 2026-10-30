/**
 * @file CastTemplates.h
 * @brief UObject 포인터의 타입 검사 변환 템플릿의 정의입니다.
 * @ingroup CoreUObject
 */

#pragma once

namespace CoreUObject {

    /**
     * @brief null 및 리플렉션 타입을 검사한 뒤 요청한 UObject 파생 포인터로 변환합니다.
     */
    template <typename T>
    T* Cast(CoreUObject::UObject* Object) {
        static_assert(std::is_base_of_v<CoreUObject::UObject, T>);
        static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

        return Object != nullptr && Object->IsA(T::StaticClass()) ? static_cast<T*>(Object) : nullptr;
    }

    /**
     * @brief null 및 리플렉션 타입을 검사한 뒤 요청한 UObject 파생 포인터로 변환합니다.
     */
    template <typename T>
    const T* Cast(const CoreUObject::UObject* Object) {
        static_assert(std::is_base_of_v<CoreUObject::UObject, T>);
        static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

        return Object != nullptr && Object->IsA(T::StaticClass()) ? static_cast<const T*>(Object) : nullptr;
    }

}
