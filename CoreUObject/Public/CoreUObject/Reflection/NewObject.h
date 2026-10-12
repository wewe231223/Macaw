#pragma once

#include "CoreUObject/Reflection/UClass.h"
#include <utility>

class FObjectInitializer {
public:
    explicit FObjectInitializer(UClass* Class, bool ClassDefaultObject = false);
    ~FObjectInitializer();
    FObjectInitializer(const FObjectInitializer&) = delete;
    FObjectInitializer& operator=(const FObjectInitializer&) = delete;

public:
    static UClass* GetConstructingClass();
    static bool IsConstructingDefault();

private:
    UClass* mPreviousClass{};
    bool mPreviousDefault{};
};

template <typename T, typename... TArguments>
std::unique_ptr<T> NewObject(TArguments&&... Arguments);

template <typename T>
std::unique_ptr<UObject> CreateNativeInstance();

template <typename T, typename... TArguments>
std::unique_ptr<T> NewObject(TArguments&&... Arguments) {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    const FObjectInitializer Initializer{T::StaticClass()};

    return std::make_unique<T>(std::forward<TArguments>(Arguments)...);
}

template <typename T>
std::unique_ptr<UObject> CreateNativeInstance() {
    if constexpr (std::is_abstract_v<T> || !std::is_default_constructible_v<T>) {
        return nullptr;
    } else {
        return NewObject<T>();
    }
}
