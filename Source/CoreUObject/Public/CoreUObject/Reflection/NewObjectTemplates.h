#pragma once

template <typename T, typename... TArguments>
std::unique_ptr<T> NewObject(FObjectRegistry& Registry, TArguments&&... Arguments) {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    T::StaticClass();

    auto Object{std::make_unique<T>(std::forward<TArguments>(Arguments)...)};

    if (!Registry.Register(*Object)) {
        return nullptr;
    }

    return Object;
}

template <typename T>
std::unique_ptr<UObject> CreateNativeInstance() {
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    if constexpr (std::is_abstract_v<T> || !std::is_default_constructible_v<T>) {
        return nullptr;
    } else {
        T::StaticClass();

        return std::make_unique<T>();
    }
}

template <typename T>
UClass* DefineNativeClass(FString Name, UClass* (*Parent)(), EClassFlags Flags) {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    if constexpr (std::is_abstract_v<T>) {
        Flags = Flags | EClassFlags::Abstract;
    }

    const FReflectionRegistry::FFactory Factory{std::is_abstract_v<T> || !std::is_default_constructible_v<T> || (static_cast<Uint32>(Flags) & static_cast<Uint32>(EClassFlags::NoFactory | EClassFlags::Abstract)) != 0 ? nullptr : &CreateNativeInstance<T>};

    return FReflectionRegistry::DefineClass(std::move(Name), Parent, GetNativeType<T>(), Flags, Factory, &BuildNativeReflection<T, UClass>);
}
