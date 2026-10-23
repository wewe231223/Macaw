/**
 * @file NewObjectTemplates.h
 * @brief UObject 생성과 클래스 메타데이터 등록 템플릿의 정의입니다.
 * @ingroup CoreUObject
 */

#pragma once

/**
 * @brief UObject 파생 객체를 생성하고 지정한 객체 레지스트리에 등록합니다.
 */
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

/**
 * @brief 구체적이고 기본 생성 가능한 네이티브 타입의 인스턴스를 생성합니다.
 */
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

/**
 * @brief 네이티브 클래스의 상속, 생성 정책과 멤버 빌더를 등록합니다.
 */
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
