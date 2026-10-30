/**
 * @file NewObjectTemplates.h
 * @brief UObject 생성과 클래스 메타데이터 등록 템플릿의 정의입니다.
 * @ingroup CoreUObject
 */

#pragma once

namespace CoreUObject {

    /**
     * @brief UObject 파생 객체를 생성하고 전역 레지스트리에 자동 등록합니다.
     */
    template <typename T, typename... TArguments>
    std::unique_ptr<T> NewObject(TArguments&&... Arguments) {
        static_assert(std::is_base_of_v<CoreUObject::UObject, T>);
        static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

        T::StaticClass();
        return std::make_unique<T>(std::forward<TArguments>(Arguments)...);
    }

    /**
     * @brief 구체적이고 기본 생성 가능한 네이티브 타입의 인스턴스를 생성합니다.
     */
    template <typename T>
    std::unique_ptr<CoreUObject::UObject> CreateNativeInstance() {
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
    CoreUObject::UClass* DefineNativeClass(Core::FString Name, CoreUObject::UClass* (*Parent)(), CoreUObject::EClassFlags Flags) {
        static_assert(std::is_base_of_v<CoreUObject::UObject, T>);
        static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

        if constexpr (std::is_abstract_v<T>) {
            Flags = Flags | CoreUObject::EClassFlags::Abstract;
        }

        const CoreUObject::FReflectionRegistry::FFactory Factory{std::is_abstract_v<T> || !std::is_default_constructible_v<T> || (static_cast<Core::Uint32>(Flags) & static_cast<Core::Uint32>(CoreUObject::EClassFlags::NoFactory | CoreUObject::EClassFlags::Abstract)) != 0 ? nullptr : &CoreUObject::CreateNativeInstance<T>};

        return CoreUObject::FReflectionRegistry::DefineClass(std::move(Name), Parent, CoreUObject::GetNativeType<T>(), Flags, Factory, &CoreUObject::BuildNativeReflection<T, CoreUObject::UClass>);
    }

}
