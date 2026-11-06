/**
 * @file ReflectionMacros.h
 * @brief 코드 생성에 사용하는 리플렉션 선언 또는 구현 매크로입니다.
 * @ingroup Core
 */

#pragma once

#include <type_traits>

#include "Core/Types.h"
#include "Core/Reflection/EPropertyFlags.h"

namespace CoreUObject {

    class UClass;
    class UScriptStruct;
    class FReflectionBuilder;

    /**
     * @brief 타입이 생성한 멤버 등록 함수를 리플렉션 빌더로 실행합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @tparam TMetadata 프로퍼티를 등록할 메타데이터 타입입니다.
     * @param[in,out] Information 프로퍼티를 등록할 클래스 또는 구조체 메타데이터입니다.
     */
    template <typename T, typename TMetadata>
    void BuildNativeReflection(TMetadata* Information);

    template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
    void RegisterProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags = Core::EPropertyFlags::None);

    template <auto Member, auto Getter = nullptr>
    void RegisterReadOnlyProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags = Core::EPropertyFlags::None);

    template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
    void RegisterArrayProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags = Core::EPropertyFlags::None);

}

/**
 * @brief 리플렉션의 소유 타입과 선택적인 부모 타입 별칭을 선언합니다.
 * @param Type 리플렉션할 소유 타입입니다.
 */
#define ReflectTypes(Type, ...) using ReflectionOwner = Type __VA_OPT__(; using Super = __VA_ARGS__)
/**
 * @brief 최상위 UObject 타입의 정적 및 가상 클래스 조회 함수를 선언합니다.
 * @param Type 리플렉션할 소유 타입입니다.
 */
#define ReflectRootClass(Type, ...) static_assert(std::is_same_v<ReflectionOwner, Type>); static CoreUObject::UClass* StaticClass(); virtual CoreUObject::UClass* GetClass() const
/**
 * @brief UObject 파생 타입의 정적 및 가상 클래스 조회 함수를 선언합니다.
 * @param Type 리플렉션할 소유 타입입니다.
 */
#define ReflectClass(Type, ...) static_assert(std::is_same_v<ReflectionOwner, Type>); static CoreUObject::UClass* StaticClass(); CoreUObject::UClass* GetClass() const override
/**
 * @brief 일반 네이티브 타입의 정적 구조체 메타데이터 조회 함수를 선언합니다.
 * @param Type 리플렉션할 소유 타입입니다.
 */
#define ReflectStruct(Type) static_assert(std::is_same_v<ReflectionOwner, Type>); static CoreUObject::UScriptStruct* StaticStruct()
/**
 * @brief 생성된 멤버 등록 함수와 네이티브 빌더에 필요한 접근 권한을 선언합니다.
 */
#define ReflectMembers() template <typename T, typename TMetadata> friend void CoreUObject::BuildNativeReflection(TMetadata* Information); static void RegisterReflectionMembers(CoreUObject::FReflectionBuilder& Information)

/**
 * @brief 생성기가 직접 읽고 쓰는 멤버 프로퍼티와 정책을 표시합니다.
 * @param Member 리플렉션 생성기에 전달할 인수입니다.
 * @param Name 조회하거나 기록할 이름입니다.
 * @param Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
 */
#define ReflectProperty(Member, Name, Flags)
/**
 * @brief 생성기가 getter와 setter를 호출하는 프로퍼티를 표시합니다.
 * @param Member 리플렉션 생성기에 전달할 인수입니다.
 * @param Getter 리플렉션 생성기에 전달할 인수입니다.
 * @param Setter 리플렉션 생성기에 전달할 인수입니다.
 * @param Name 조회하거나 기록할 이름입니다.
 * @param Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
 */
#define ReflectAccessor(Member, Getter, Setter, Name, Flags)
/**
 * @brief 직접 읽기와 지정한 setter를 사용하는 프로퍼티를 표시합니다.
 * @param Member 리플렉션 생성기에 전달할 인수입니다.
 * @param Setter 리플렉션 생성기에 전달할 인수입니다.
 * @param Name 조회하거나 기록할 이름입니다.
 * @param Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
 */
#define ReflectSetter(Member, Setter, Name, Flags)
/**
 * @brief 직접 읽기 및 로드 전용 직접 쓰기를 사용하는 읽기 전용 프로퍼티를 표시합니다.
 * @param Member 리플렉션 생성기에 전달할 인수입니다.
 * @param Name 조회하거나 기록할 이름입니다.
 * @param Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
 */
#define ReflectReadOnlyProperty(Member, Name, Flags)
/**
 * @brief getter로 읽고 로드할 때만 멤버에 쓰는 프로퍼티를 표시합니다.
 * @param Member 리플렉션 생성기에 전달할 인수입니다.
 * @param Getter 리플렉션 생성기에 전달할 인수입니다.
 * @param Name 조회하거나 기록할 이름입니다.
 * @param Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
 */
#define ReflectReadOnlyAccessor(Member, Getter, Name, Flags)
/**
 * @brief 멤버에 직접 읽고 쓰는 TArray 프로퍼티를 표시합니다.
 * @param Member 리플렉션 생성기에 전달할 인수입니다.
 * @param Name 조회하거나 기록할 이름입니다.
 * @param Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
 */
#define ReflectArray(Member, Name, Flags)
/**
 * @brief const TArray& getter와 배열 전체를 받는 setter를 사용하는 TArray 프로퍼티를 표시합니다.
 * @param Member 리플렉션 생성기에 전달할 인수입니다.
 * @param Getter 리플렉션 생성기에 전달할 인수입니다.
 * @param Setter 리플렉션 생성기에 전달할 인수입니다.
 * @param Name 조회하거나 기록할 이름입니다.
 * @param Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
 */
#define ReflectArrayAccessor(Member, Getter, Setter, Name, Flags)

/**
 * @brief 바로 앞에 선언한 멤버 함수를 C#에 노출합니다. 내용 없이 생성기만 읽습니다.
 * @param Name 함수 이름입니다.
 * @param ... Internal을 넘기면 C# 쪽 접근 수준이 internal이 됩니다.
 */
#define ReflectFunction(Name, ...)

/**
 * @brief 바로 앞에 정의한 열거형을 앞의 E를 뗀 이름으로 C#에 생성합니다. 내용 없이 생성기만 읽습니다.
 * @param Type 열거형 이름입니다.
 */
#define ReflectScriptEnum(Type)
