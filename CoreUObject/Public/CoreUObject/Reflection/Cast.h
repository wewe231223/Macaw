#pragma once

#include "CoreUObject/Reflection/UClass.h"

template <typename T>
T* Cast(UObject* Object);

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
