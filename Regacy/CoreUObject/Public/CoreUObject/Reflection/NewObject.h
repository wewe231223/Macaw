/**
 * @file NewObject.h
 * @brief 객체 생성 문맥, 타입별 생성 함수와 네이티브 클래스 등록을 제공합니다.
 */

#pragma once

#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include <utility>

/**
 * @brief 현재 스레드의 객체 생성 클래스와 기본 객체 상태를 범위 동안 설정하는 도우미입니다.
 * @note 중첩 생성 시 이전 문맥을 저장하고 소멸 시 복원합니다.
 */
class FObjectInitializer {
public:
    /**
     * @brief 현재 스레드의 객체 생성 문맥을 설정합니다.
     * @param Class 생성할 실제 클래스 메타데이터입니다.
     * @param ClassDefaultObject 클래스 기본 객체 생성 여부입니다.
     * @note 같은 클래스의 기본 객체 생성 문맥에 중첩되면 기존 기본 객체 상태를 유지합니다.
     */
    explicit FObjectInitializer(UClass* Class, bool ClassDefaultObject = false);

    /**
     * @brief 현재 스레드의 이전 생성 문맥을 복원합니다.
     */
    ~FObjectInitializer();

    /**
     * @brief 생성 문맥 범위의 복사 생성을 금지합니다.
     */
    FObjectInitializer(const FObjectInitializer&) = delete;

    /**
     * @brief 생성 문맥 범위의 복사 대입을 금지합니다.
     */
    FObjectInitializer& operator=(const FObjectInitializer&) = delete;

public:
    /**
     * @brief 현재 스레드에서 생성 중인 클래스 정보를 반환합니다.
     * @return 생성 문맥의 클래스입니다. 문맥이 없으면 nullptr입니다.
     */
    static UClass* GetConstructingClass();

    /**
     * @brief 현재 스레드가 클래스 기본 객체를 생성 중인지 반환합니다.
     * @return 기본 객체 생성 문맥이면 true입니다.
     */
    static bool IsConstructingDefault();

private:
    /**
     * @brief 범위 종료 시 복원할 이전 클래스입니다.
     */
    UClass* mPreviousClass{};

    /**
     * @brief 범위 종료 시 복원할 이전 기본 객체 생성 상태입니다.
     */
    bool mPreviousDefault{};
};

/**
 * @brief T의 클래스 생성 문맥을 설정하고 생성자 인수를 전달해 객체를 만듭니다.
 * @tparam T 자체 리플렉션 정보를 선언한 UObject 파생 타입입니다.
 * @tparam TArguments 생성자 인수 타입 목록입니다.
 * @param Arguments 완전 전달할 생성자 인수입니다.
 * @return 생성 객체의 단독 소유권입니다.
 * @note 객체 저장소 등록과 Outer 설정은 별도로 수행합니다.
 */
template <typename T, typename... TArguments>
std::unique_ptr<T> NewObject(TArguments&&... Arguments);

/**
 * @brief 기본 생성 가능한 구체 타입을 NewObject로 생성합니다.
 * @tparam T 생성할 UObject 파생 타입입니다.
 * @return 생성 객체입니다. 추상 타입이거나 기본 생성할 수 없으면 nullptr입니다.
 */
template <typename T>
std::unique_ptr<UObject> CreateNativeInstance();

/**
 * @brief T의 크기, 정렬과 생성 팩터리를 사용해 클래스 메타데이터를 정의합니다.
 * @tparam T 메타데이터를 정의할 네이티브 객체 타입입니다.
 * @param Name 클래스 이름입니다.
 * @param Parent 기반 클래스 정보를 반환하는 함수입니다. nullptr이면 기반 클래스가 없습니다.
 * @param Flags 클래스 속성 플래그입니다.
 * @param Builder 프로퍼티와 함수를 등록할 빌더입니다.
 * @return 등록된 클래스 메타데이터입니다.
 * @note NoFactory이면 팩터리를 등록하지 않습니다.
 */
template <typename T>
UClass* DefineNativeClass(FName Name, UClass* (*Parent)(), EClassFlags Flags = EClassFlags::None, FReflectionRegistry::FClassBuilder Builder = nullptr);

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

template <typename T>
UClass* DefineNativeClass(FName Name, UClass* (*Parent)(), EClassFlags Flags, FReflectionRegistry::FClassBuilder Builder) {
    const FReflectionRegistry::FFactory Factory{(static_cast<Uint32>(Flags) & static_cast<Uint32>(EClassFlags::NoFactory)) != 0 ? nullptr : &CreateNativeInstance<T>};

    return FReflectionRegistry::DefineClass(Name, Parent, sizeof(T), alignof(T), Flags, Factory, Builder);
}
