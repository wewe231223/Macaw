#pragma once

template <typename T>
bool UObject::IsA() const {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return IsA(T::StaticClass());
}
