/**
 * @file Cast.h
 * @brief 리플렉션 상속 정보를 검사하는 UObject 포인터 변환을 제공합니다.
 */

#pragma once

#include "CoreUObject/Reflection/UClass.h"

/**
 * @brief 객체가 T 또는 그 파생 타입이면 T 포인터로 변환합니다.
 * @tparam T 자체 리플렉션 정보를 선언한 UObject 파생 타입입니다.
 * @param Object 변환할 객체입니다.
 * @return 타입이 호환되면 객체 포인터이고, null 또는 타입 불일치이면 nullptr입니다.
 */
template <typename T>
T* Cast(UObject* Object);

/**
 * @brief 상수 객체 포인터를 타입 검사 후 상수 T 포인터로 변환합니다.
 * @tparam T 자체 리플렉션 정보를 선언한 UObject 파생 타입입니다.
 * @param Object 변환할 객체입니다.
 * @return 타입이 호환되면 상수 객체 포인터이고, null 또는 타입 불일치이면 nullptr입니다.
 */
template <typename T>
const T* Cast(const UObject* Object);

template <typename T>
T* Cast(UObject* Object) {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return Object != nullptr && Object->IsA(T::StaticClass()) ? static_cast<T*>(Object) : nullptr;
}

template <typename T>
const T* Cast(const UObject* Object) {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return Object != nullptr && Object->IsA(T::StaticClass()) ? static_cast<const T*>(Object) : nullptr;
}
