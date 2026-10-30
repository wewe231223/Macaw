/**
 * @file ReflectionMacros.h
 * @brief 클래스·구조체·열거형과 멤버의 네이티브 리플렉션 선언 매크로를 제공합니다.
 * @note 프로퍼티와 함수 등록 매크로는 ReflectTypes와 ReflectMembers 사이에서 사용합니다.
 */

#pragma once

#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/NativeReflection.h"

class UScriptStruct;
class UEnum;

/**
 * @brief 리플렉션 소유 타입, 선택적 Super 별칭과 멤버 검색 시작 줄을 선언합니다.
 * @param Type 자체 리플렉션 소유 타입입니다.
 * @note 가변 인수에는 기반 타입을 지정합니다. 멤버 등록 매크로보다 앞에 선언하고 ReflectMembers로 등록 범위를 닫습니다.
 */
#define ReflectTypes(Type, ...) using ReflectionOwner = Type __VA_OPT__(; using Super = __VA_ARGS__); enum : std::size_t { \
    ReflectionBegin = __LINE__ \
}

/**
 * @brief 기반 클래스를 갖는 타입의 StaticClass 함수와 네이티브 정의를 생성합니다.
 * @param Type 등록할 클래스 타입입니다.
 * @note Super::StaticClass를 기반 조회 함수로 사용합니다. 가변 인수에는 EClassFlags 비트를 지정합니다.
 */
#define ReflectClass(Type, ...) static UClass* StaticClass() { \
    return DefineNativeClass<Type>(FName{#Type}, &Super::StaticClass, EClassFlags::None __VA_OPT__(| __VA_ARGS__), &BuildNativeReflection<Type, UClass>); \
}

/**
 * @brief 기반 클래스가 없는 타입의 StaticClass 함수와 네이티브 정의를 생성합니다.
 * @param Type 등록할 기반 클래스 타입입니다.
 * @note 가변 인수에는 EClassFlags 비트를 지정합니다.
 */
#define ReflectRootClass(Type, ...) static UClass* StaticClass() { \
    return DefineNativeClass<Type>(FName{#Type}, nullptr, EClassFlags::None __VA_OPT__(| __VA_ARGS__), &BuildNativeReflection<Type, UClass>); \
}

/**
 * @brief 네이티브 구조체의 StaticStruct 함수와 멤버 등록 빌더를 생성합니다.
 * @param Type 등록할 구조체 타입입니다.
 */
#define ReflectStruct(Type) static UScriptStruct* StaticStruct() { \
    return FReflectionRegistry::DefineStruct(FName{#Type}, GetNativeType<Type>(), &BuildNativeReflection<Type, UScriptStruct>); \
}

/**
 * @brief 열거형 값 등록 빌더와 StaticEnum 뒤에 타입 이름을 붙인 조회 함수를 생성합니다.
 * @param Type 등록할 열거형 타입입니다.
 * @note 가변 인수에는 ReflectEnumValue로 만든 이름·값 쌍을 지정합니다.
 */
#define ReflectEnum(Type, ...) inline void Build##Type##Reflection(UEnum& Information) { \
    FReflectionRegistry::AddEnumValues(Information, {__VA_ARGS__}); \
} \
inline UEnum* StaticEnum##Type() { \
    return FReflectionRegistry::DefineEnum(FName{#Type}, GetNativeType<Type>(), &Build##Type##Reflection); \
}

/**
 * @brief 열거형 항목의 표시 이름과 Int64 값 쌍을 만듭니다.
 * @param Type 열거형 타입입니다.
 * @param Value Type에 선언된 항목 이름입니다.
 */
#define ReflectEnumValue(Type, Value) std::pair<FName, Int64>{FName{#Value}, static_cast<Int64>(Type::Value)}

/**
 * @brief 데이터 멤버를 직접 읽고 쓰는 프로퍼티의 줄별 등록 함수를 선언합니다.
 * @param Member 등록할 데이터 멤버 이름입니다.
 * @param Name 리플렉션 프로퍼티 이름입니다.
 * @param Flags EPropertyFlags 정책 비트입니다.
 * @note 가변 인수에는 에디터 옵션이나 직렬화 함수를 지정합니다.
 */
#define ReflectProperty(Member, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}

/**
 * @brief 읽기·쓰기 접근자를 사용하는 프로퍼티를 등록합니다.
 * @param Member 실제 데이터 멤버 이름입니다.
 * @param Getter 읽기 접근자 이름입니다.
 * @param Setter 쓰기 접근자 이름입니다.
 * @param Name 리플렉션 프로퍼티 이름입니다.
 * @param Flags EPropertyFlags 정책 비트입니다.
 * @note 가변 인수에는 프로퍼티 옵션을 지정합니다. 참조 방문은 실제 데이터 멤버를 사용합니다.
 */
#define ReflectAccessor(Member, Getter, Setter, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member, &ReflectionOwner::Getter, &ReflectionOwner::Setter>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}

/**
 * @brief 멤버를 직접 읽고 지정한 쓰기 접근자로 변경하는 프로퍼티를 등록합니다.
 * @param Member 실제 데이터 멤버 이름입니다.
 * @param Setter 쓰기 접근자 이름입니다.
 * @param Name 리플렉션 프로퍼티 이름입니다.
 * @param Flags EPropertyFlags 정책 비트입니다.
 * @note 가변 인수에는 프로퍼티 옵션을 지정합니다.
 */
#define ReflectSetter(Member, Setter, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member, nullptr, &ReflectionOwner::Setter>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}

/**
 * @brief 직접 멤버 읽기를 제공하는 읽기 전용 프로퍼티를 등록합니다.
 * @param Member 데이터 멤버 이름입니다.
 * @param Name 리플렉션 프로퍼티 이름입니다.
 * @param Flags 추가 EPropertyFlags 비트입니다. ReadOnly를 자동으로 추가합니다.
 * @note 가변 인수에는 프로퍼티 옵션을 지정합니다.
 */
#define ReflectReadOnlyProperty(Member, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeReadOnlyProperty<&ReflectionOwner::Member>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}

/**
 * @brief 읽기 접근자를 사용하는 읽기 전용 프로퍼티를 등록합니다.
 * @param Member 실제 데이터 멤버 이름입니다.
 * @param Getter 읽기 접근자 이름입니다.
 * @param Name 리플렉션 프로퍼티 이름입니다.
 * @param Flags 추가 EPropertyFlags 비트입니다. ReadOnly를 자동으로 추가합니다.
 * @note 가변 인수에는 프로퍼티 옵션을 지정합니다.
 */
#define ReflectReadOnlyAccessor(Member, Getter, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeReadOnlyProperty<&ReflectionOwner::Member, &ReflectionOwner::Getter>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}

/**
 * @brief 실제 데이터 멤버를 직렬화하는 프로퍼티 콜백을 선택합니다.
 * @param Member 직렬화할 데이터 멤버 이름입니다.
 */
#define ReflectSerializer(Member) &SerializeMemberProperty<&ReflectionOwner::Member>
/**
 * @brief 멤버 함수의 호출 어댑터와 인수·반환 타입을 등록합니다.
 * @param Method 등록할 멤버 함수 이름입니다.
 * @note 가변 인수에는 모든 인수의 이름을 선언 순서로 지정합니다. 오버로드된 함수는 멤버 포인터를 유일하게 결정할 수 있어야 합니다.
 */
#define ReflectFunction(Method, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterFunction<&ReflectionOwner::Method>(Information, FName{#Method} __VA_OPT__(, {__VA_ARGS__})); \
}

/**
 * @brief 에디터 분류와 표시 이름을 설정하는 프로퍼티 옵션을 생성합니다.
 * @param Category 분류 문자열입니다.
 * @param DisplayName 표시 이름 문자열입니다.
 */
#define ReflectEditor(Category, DisplayName) [](FProperty& Property) { \
    Property.SetEditorMetadata(FString{Category}, FString{DisplayName}, Property.GetMinimum(), Property.GetMaximum(), Property.GetSpeed()); \
}

/**
 * @brief 정적 수치 편집 범위를 설정하는 프로퍼티 옵션을 생성합니다.
 * @param Minimum 정적 하한입니다.
 * @param Maximum 정적 상한입니다.
 * @note 값 쓰기 시 자동 범위 제한은 수행하지 않습니다.
 */
#define ReflectRange(Minimum, Maximum) [](FProperty& Property) { \
    Property.SetEditorMetadata(Property.GetCategory(), Property.GetDisplayName(), Minimum, Maximum, Property.GetSpeed()); \
}

/**
 * @brief 수치 편집 속도를 설정하는 프로퍼티 옵션을 생성합니다.
 * @param Speed 수치 편집 속도입니다.
 */
#define ReflectSpeed(Speed) [](FProperty& Property) { \
    Property.SetEditorMetadata(Property.GetCategory(), Property.GetDisplayName(), Property.GetMinimum(), Property.GetMaximum(), Speed); \
}

/**
 * @brief 무인수 상수 멤버 함수의 결과를 double로 읽는 동적 범위 콜백을 선택합니다.
 * @param Method 동적 범위를 반환하는 멤버 함수 이름입니다.
 */
#define ReflectBound(Method) &ReadPropertyMetadata<&ReflectionOwner::Method, double>
/**
 * @brief 객체별 동적 편집 범위를 설정하는 프로퍼티 옵션을 생성합니다.
 * @param Minimum 동적 하한 콜백입니다. nullptr를 허용합니다.
 * @param Maximum 동적 상한 콜백입니다. nullptr를 허용합니다.
 */
#define ReflectBounds(Minimum, Maximum) [](FProperty& Property) { \
    Property.SetDynamicBounds(Minimum, Maximum); \
}

/**
 * @brief 객체별 편집 허용 조건을 설정하는 프로퍼티 옵션을 생성합니다.
 * @param Method bool로 변환 가능한 결과를 반환하는 무인수 상수 멤버 함수 이름입니다.
 */
#define ReflectCondition(Method) [](FProperty& Property) { \
    Property.SetEditCondition(&ReadPropertyMetadata<&ReflectionOwner::Method, bool>); \
}

/**
 * @brief 객체별 이름 선택 목록을 제공하는 프로퍼티 옵션을 생성합니다.
 * @param Method TArray<FName>으로 변환 가능한 목록을 반환하는 무인수 상수 멤버 함수 이름입니다.
 */
#define ReflectOptions(Method) [](FProperty& Property) { \
    Property.SetOptionsGetter(&ReadPropertyMetadata<&ReflectionOwner::Method, TArray<FName>>); \
}

/**
 * @brief 에셋 선택기의 클래스 정보를 설정하는 프로퍼티 옵션을 생성합니다.
 * @param Type StaticClass를 제공하는 에셋 클래스 타입입니다.
 */
#define ReflectAsset(Type) [](FProperty& Property) { \
    Property.SetAssetClass(Type::StaticClass()); \
}

/**
 * @brief 객체 후보의 선택 허용 필터를 설정하는 프로퍼티 옵션을 생성합니다.
 * @param Method 객체 포인터 인수를 받아 bool을 반환하는 상수 멤버 함수 이름입니다.
 */
#define ReflectFilter(Method) [](FProperty& Property) { \
    Property.SetObjectFilter(&FilterPropertyObject<&ReflectionOwner::Method>); \
}

/**
 * @brief 아카이브 필드 이름을 지정하는 프로퍼티 옵션을 생성합니다.
 * @param Name 직렬화 이름 문자열입니다.
 */
#define ReflectSerializationName(Name) [](FProperty& Property) { \
    Property.SetSerializationName(FString{Name}); \
}

/**
 * @brief 직렬화 객체 범위 이름을 지정하는 프로퍼티 옵션을 생성합니다.
 * @param Scope 범위 이름 문자열입니다. 빈 문자열이면 현재 범위를 사용합니다.
 */
#define ReflectSerializationScope(Scope) [](FProperty& Property) { \
    Property.SetSerializationScope(FString{Scope}); \
}

/**
 * @brief ReflectTypes부터 현재 줄 직전까지의 멤버 등록 함수를 실행하는 진입점을 선언합니다.
 * @note 리플렉션 멤버 등록 매크로들 뒤에 선언합니다.
 */
#define ReflectMembers() friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, 0>, FReflectionBuilder& Information) { \
    RegisterReflectionMembers<ReflectionOwner, ReflectionBegin, __LINE__>(Information); \
}
