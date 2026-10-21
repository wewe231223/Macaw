/**
 * @file NativeReflection.h
 * @brief 멤버 포인터와 매크로 등록 정보를 네이티브 프로퍼티·함수 메타데이터로 연결합니다.
 */
#pragma once

#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "Core/Base/ErrorHandler.h"
#include <span>
#include <initializer_list>
#include <tuple>

class UStruct;

/**
 * @brief 클래스 또는 구조체에 프로퍼티와 호출 정보를 추가하는 빌더입니다.
 */
class FReflectionBuilder {
public:
    /**
     * @brief 타입 소거 인수를 받아 객체 멤버를 호출하는 어댑터 타입입니다.
     */
    using FInvoker = bool (*)(UObject&, std::span<const FPropertyValue>, FPropertyValue&);

public:
    /**
     * @brief 클래스 멤버 등록 빌더를 초기화합니다.
     * @param Class 프로퍼티와 함수를 추가할 클래스입니다.
     */
    explicit FReflectionBuilder(UClass& Class);
    /**
     * @brief 구조체 프로퍼티 등록 빌더를 초기화합니다.
     * @param Struct 프로퍼티를 추가할 구조체입니다.
     */
    explicit FReflectionBuilder(UScriptStruct& Struct);

public:
    /**
     * @brief 프로퍼티를 등록할 소유 메타데이터를 반환합니다.
     * @return 클래스 또는 구조체 정보입니다.
     */
    UStruct* GetOwner() const;
    /**
     * @brief 소유 타입에 프로퍼티의 소유권을 전달합니다.
     * @param Property 등록할 프로퍼티입니다.
     */
    void AddProperty(std::unique_ptr<FProperty> Property);
    /**
     * @brief 클래스에 인수·반환 타입과 호출 어댑터를 갖는 함수를 추가합니다.
     * @param Name 함수 이름입니다.
     * @param Invoker 네이티브 호출 어댑터입니다.
     * @param ParameterNames 선언 순서의 인수 이름입니다.
     * @param ParameterTypes 선언 순서의 정확한 인수 타입입니다.
     * @param ReturnType 반환 타입입니다. nullptr이면 void입니다.
     * @pre 클래스용 빌더여야 하며 인수 이름과 타입의 개수가 같아야 합니다.
     */
    void AddFunction(FName Name, FInvoker Invoker, std::initializer_list<FName> ParameterNames, std::initializer_list<const FNativeType*> ParameterTypes, const FNativeType* ReturnType);

private:
    /**
     * @brief 프로퍼티를 등록하는 소유 메타데이터입니다.
     */
    UStruct* mOwner{};
    /**
     * @brief 함수 등록을 허용할 클래스입니다. 구조체 빌더에서는 nullptr입니다.
     */
    UClass* mClass{};
};

/**
 * @brief 타입과 소스 줄 번호로 매크로의 멤버 등록 오버로드를 구별하는 태그입니다.
 * @tparam T 리플렉션 소유 타입입니다.
 * @tparam Line 등록 매크로의 소스 줄 번호입니다. 0은 전체 멤버 등록 진입점입니다.
 */
template <typename T, std::size_t Line>
struct TReflectionMember {
};

/**
 * @brief 해당 줄 번호의 멤버 등록 오버로드가 있으면 호출합니다.
 * @tparam T 리플렉션 소유 타입입니다.
 * @tparam Line 검사할 소스 줄 번호입니다.
 * @param Builder 멤버 등록 빌더입니다.
 * @note 등록 오버로드가 없는 줄은 건너뜁니다.
 */
template <typename T, std::size_t Line>
void RegisterReflectionLine(FReflectionBuilder& Builder);

/**
 * @brief Begin에 각 인덱스를 더한 줄의 멤버 등록을 펼쳐 실행합니다.
 * @tparam T 리플렉션 소유 타입입니다.
 * @tparam Begin 줄 범위의 시작 번호입니다.
 * @tparam Indices 시작 번호에 더할 컴파일 시 인덱스 목록입니다.
 * @param Builder 멤버 등록 빌더입니다.
 */
template <typename T, std::size_t Begin, std::size_t... Indices>
void RegisterReflectionLines(FReflectionBuilder& Builder, std::index_sequence<Indices...>);

/**
 * @brief [Begin, End) 줄 범위의 리플렉션 멤버를 등록합니다.
 * @tparam T 리플렉션 소유 타입입니다.
 * @tparam Begin 시작 줄 번호입니다.
 * @tparam End 제외할 끝 줄 번호입니다.
 * @param Builder 멤버 등록 빌더입니다.
 * @pre End는 Begin 이상이어야 합니다.
 */
template <typename T, std::size_t Begin, std::size_t End>
void RegisterReflectionMembers(FReflectionBuilder& Builder);

/**
 * @brief 멤버 함수 시그니처에서 인수와 반환 타입을 추출해 등록합니다.
 * @tparam Method 등록할 멤버 함수 포인터입니다.
 * @tparam Indices 선언 순서의 인수 인덱스 목록입니다.
 * @param Owner 클래스 멤버 등록 빌더입니다.
 * @param Name 함수 이름입니다.
 * @param ParameterNames 선언 순서의 인수 이름입니다.
 */
template <auto Method, std::size_t... Indices>
void RegisterFunctionParameters(FReflectionBuilder& Owner, FName Name, std::initializer_list<FName> ParameterNames, std::index_sequence<Indices...>);

/**
 * @brief 타입의 줄 번호 0 등록 진입점을 호출해 멤버 정보를 구축합니다.
 * @tparam T 리플렉션 소유 타입입니다.
 * @tparam TMetadata UClass 또는 UScriptStruct 메타데이터 타입입니다.
 * @param Information 멤버를 추가할 메타데이터입니다.
 * @note 전체 등록 진입점이 없는 타입은 멤버를 추가하지 않습니다.
 */
template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata& Information);

/**
 * @brief 데이터 멤버 포인터의 소유 타입과 값 타입을 추출하는 특성의 기본 선언입니다.
 * @tparam T 데이터 멤버 포인터 타입입니다.
 */
template <typename T>
struct TMemberPointerTraits;

/**
 * @brief 데이터 멤버 포인터에서 소유 타입과 값 타입을 추출합니다.
 * @tparam TValue 데이터 멤버 타입입니다.
 * @tparam TOwner 멤버를 선언한 타입입니다.
 */
template <typename TValue, typename TOwner>
struct TMemberPointerTraits<TValue TOwner::*> {
public:
    /**
     * @brief 데이터 멤버를 선언한 소유 타입입니다.
     */
    using FOwner = TOwner;
    /**
     * @brief 데이터 멤버의 네이티브 값 타입입니다.
     */
    using FValue = TValue;
};

/**
 * @brief 멤버 함수 포인터의 소유 타입, 반환 타입과 인수 목록 특성의 기본 선언입니다.
 * @tparam T 멤버 함수 포인터 타입입니다.
 */
template <typename T>
struct TFunctionPointerTraits;

/**
 * @brief 멤버 함수 포인터의 소유 타입, 반환 타입과 인수 튜플을 추출합니다.
 * @tparam TResult 함수 반환 타입입니다.
 * @tparam TOwner 멤버 함수 소유 타입입니다.
 * @tparam TArguments 선언 순서의 인수 타입 목록입니다.
 */
template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
public:
    /**
     * @brief 멤버 함수를 선언한 소유 타입입니다.
     */
    using FOwner = TOwner;
    /**
     * @brief 멤버 함수 반환 타입입니다.
     */
    using FResult = TResult;
    /**
     * @brief 선언 순서의 인수 타입 튜플입니다.
     */
    using FArguments = std::tuple<TArguments...>;
};

/**
 * @brief const 멤버 함수 포인터에서 동일한 소유·반환·인수 특성을 추출합니다.
 * @tparam TResult 함수 반환 타입입니다.
 * @tparam TOwner 멤버 함수 소유 타입입니다.
 * @tparam TArguments 인수 타입 목록입니다.
 */
template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...) const> : TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
};

/**
 * @brief noexcept 멤버 함수 포인터에서 동일한 소유·반환·인수 특성을 추출합니다.
 * @tparam TResult 함수 반환 타입입니다.
 * @tparam TOwner 멤버 함수 소유 타입입니다.
 * @tparam TArguments 인수 타입 목록입니다.
 */
template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...) noexcept> : TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
};

/**
 * @brief const noexcept 멤버 함수 포인터에서 동일한 소유·반환·인수 특성을 추출합니다.
 * @tparam TResult 함수 반환 타입입니다.
 * @tparam TOwner 멤버 함수 소유 타입입니다.
 * @tparam TArguments 인수 타입 목록입니다.
 */
template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...) const noexcept> : TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
};

/**
 * @brief 접근자가 있으면 호출하고 없으면 데이터 멤버 값을 직접 읽습니다.
 * @tparam Member 데이터 멤버 포인터입니다.
 * @tparam Getter 선택적 읽기 접근자입니다. nullptr이면 직접 읽습니다.
 * @param Object 멤버의 소유 객체입니다.
 * @return 직접 멤버 접근 또는 접근자 호출 결과입니다.
 */
template <auto Member, auto Getter = nullptr>
decltype(auto) ReadMemberValue(const typename TMemberPointerTraits<decltype(Member)>::FOwner& Object);

/**
 * @brief 저장 공간에서 멤버나 접근자 값을 읽어 타입 소거 복사본을 만듭니다.
 * @tparam Member 데이터 멤버 포인터입니다.
 * @tparam Getter 선택적 읽기 접근자입니다.
 * @param Container 유효한 소유 타입의 저장 공간입니다.
 * @return 읽은 값의 복사본입니다.
 */
template <auto Member, auto Getter = nullptr>
FPropertyValue ReadMemberProperty(const void* Container);

/**
 * @brief 정확한 값 타입을 검사한 뒤 멤버 대입 또는 쓰기 접근자를 수행합니다.
 * @tparam Member 데이터 멤버 포인터입니다.
 * @tparam Getter 값 타입 추출에 사용하는 읽기 접근자입니다.
 * @tparam Setter 선택적 쓰기 접근자입니다. nullptr이면 직접 대입합니다.
 * @param Container 유효한 소유 타입의 저장 공간입니다.
 * @param Value 적용할 값입니다.
 * @return 타입이 맞고 쓰기가 성공하면 true입니다. bool 접근자의 반환값은 성공 여부로 사용합니다.
 */
template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
bool WriteMemberProperty(void* Container, const FPropertyValue& Value);

/**
 * @brief 실제 데이터 멤버를 직렬화하고 로드 시 멤버에 결과를 저장합니다.
 * @tparam Member 직렬화할 데이터 멤버 포인터입니다.
 * @param Container 유효한 소유 타입의 저장 공간입니다.
 * @param Archive 사용할 아카이브입니다.
 * @param Property 직렬화 이름과 Inline 정책을 제공하는 메타데이터입니다.
 * @note 리플렉션 구조체에 Inline이 있으면 상속 프로퍼티를 포함해 현재 범위에 직렬화합니다.
 */
template <auto Member>
void SerializeMemberProperty(void* Container, FArchive& Archive, const FProperty& Property);

/**
 * @brief 실제 데이터 멤버의 네이티브 객체 참조를 방문합니다.
 * @tparam Member 방문할 데이터 멤버 포인터입니다.
 * @param Container 유효한 소유 타입의 저장 공간입니다.
 * @param Visitor 발견한 객체를 처리할 콜백입니다.
 */
template <auto Member>
void VisitMemberProperty(const void* Container, const std::function<void(UObject*)>& Visitor);

/**
 * @brief 상수 소유 객체의 메타데이터 접근자를 호출하고 TResult로 변환합니다.
 * @tparam Getter 메타데이터를 반환하는 멤버 함수 포인터입니다.
 * @tparam TResult 변환할 결과 타입입니다.
 * @param Container 유효한 소유 타입의 저장 공간입니다.
 * @return 접근자 결과를 변환한 값입니다.
 */
template <auto Getter, typename TResult>
TResult ReadPropertyMetadata(const void* Container);

/**
 * @brief 후보 객체를 멤버 함수의 인수 타입으로 변환해 선택 필터를 호출합니다.
 * @tparam Method 객체 후보를 받아 bool을 반환하는 상수 멤버 함수입니다.
 * @param Container 유효한 소유 타입의 저장 공간입니다.
 * @param Candidate 필터 인수 타입과 호환되는 후보 객체입니다. nullptr를 허용합니다.
 * @return 필터의 반환값입니다.
 */
template <auto Method>
bool FilterPropertyObject(const void* Container, UObject* Candidate);

/**
 * @brief 데이터 멤버와 선택적 접근자로 읽기·쓰기 프로퍼티를 생성합니다.
 * @tparam Member 데이터 멤버 포인터입니다.
 * @tparam Getter 선택적 읽기 접근자입니다.
 * @tparam Setter 선택적 쓰기 접근자입니다.
 * @param Owner 소유 메타데이터를 제공할 빌더입니다.
 * @param Name 프로퍼티 이름입니다.
 * @param Flags 프로퍼티 정책 플래그입니다.
 * @param Serializer 선택적 직렬화 콜백입니다.
 * @return 새 프로퍼티의 소유권입니다. 빌더에 등록하는 작업은 별도로 수행합니다.
 */
template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
std::unique_ptr<FProperty> MakeProperty(FReflectionBuilder& Owner, FName Name, EPropertyFlags Flags = EPropertyFlags::None, FProperty::FSerializer Serializer = nullptr);

/**
 * @brief 읽기 함수만 제공하고 ReadOnly 플래그를 추가한 프로퍼티를 생성합니다.
 * @tparam Member 데이터 멤버 포인터입니다.
 * @tparam Getter 선택적 읽기 접근자입니다.
 * @param Owner 소유 메타데이터를 제공할 빌더입니다.
 * @param Name 프로퍼티 이름입니다.
 * @param Flags 추가 정책 플래그입니다.
 * @param Serializer 선택적 직렬화 콜백입니다.
 * @return 새 읽기 전용 프로퍼티의 소유권입니다.
 */
template <auto Member, auto Getter = nullptr>
std::unique_ptr<FProperty> MakeReadOnlyProperty(FReflectionBuilder& Owner, FName Name, EPropertyFlags Flags = EPropertyFlags::None, FProperty::FSerializer Serializer = nullptr);

/**
 * @brief 프로퍼티 옵션이 직렬화 함수이면 설정하고 그 외에는 옵션을 호출합니다.
 * @tparam TOption 직렬화 콜백 또는 프로퍼티를 받는 호출 가능 타입입니다.
 * @param Property 옵션을 적용할 프로퍼티입니다.
 * @param Option 적용할 옵션입니다.
 */
template <typename TOption>
void ApplyPropertyOption(FProperty& Property, const TOption& Option);

/**
 * @brief 기본 에디터 수치 범위와 옵션을 적용한 뒤 프로퍼티를 등록합니다.
 * @tparam TOptions 적용할 옵션 타입 목록입니다.
 * @param Owner 등록 빌더입니다.
 * @param Property 소유권을 전달할 유효한 프로퍼티입니다.
 * @param Options 지정한 순서로 적용할 옵션입니다.
 */
template <typename... TOptions>
void RegisterProperty(FReflectionBuilder& Owner, std::unique_ptr<FProperty> Property, const TOptions&... Options);

/**
 * @brief 컴파일 시 인수 인덱스로 타입 소거 인수를 풀어 멤버 함수를 호출합니다.
 * @tparam Method 호출할 멤버 함수 포인터입니다.
 * @tparam Indices 선언 순서의 인수 인덱스입니다.
 * @param Object 멤버 함수 소유 타입과 호환되는 객체입니다.
 * @param Arguments 개수와 정확한 타입 검증을 마친 인수 목록입니다.
 * @param[out] Result void가 아닌 반환값의 복사본을 저장합니다. void 호출에서는 기존 값을 유지합니다.
 * @pre 객체와 인수의 타입·개수를 호출 전에 검증해야 합니다.
 * @return 호출을 완료하면 true입니다.
 */
template <auto Method, std::size_t... Indices>
bool InvokeMemberFunction(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result, std::index_sequence<Indices...>);

/**
 * @brief 함수 시그니처의 인수 개수로 인덱스 목록을 생성해 멤버 함수를 호출합니다.
 * @tparam Method 호출할 멤버 함수 포인터입니다.
 * @param Object 소유 타입과 호환되는 객체입니다.
 * @param Arguments 정확한 타입과 개수를 갖는 인수 목록입니다.
 * @param[out] Result void가 아닌 반환값을 저장합니다. void 호출에서는 기존 값을 유지합니다.
 * @pre 객체와 인수의 타입·개수를 호출 전에 검증해야 합니다.
 * @return 호출을 완료하면 true입니다.
 */
template <auto Method>
bool InvokeMemberFunction(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result);

/**
 * @brief 인수 이름 개수를 검사하고 멤버 함수의 호출 어댑터와 시그니처를 등록합니다.
 * @tparam Method 등록할 멤버 함수 포인터입니다.
 * @param Owner 클래스 멤버 등록 빌더입니다.
 * @param Name 리플렉션 함수 이름입니다.
 * @param ParameterNames 모든 인수의 이름을 선언 순서로 제공합니다.
 * @note 함수 인수 개수와 이름 개수가 다르면 Critical 오류를 보고합니다.
 */
template <auto Method>
void RegisterFunction(FReflectionBuilder& Owner, FName Name, std::initializer_list<FName> ParameterNames = {});

template <auto Member, auto Getter>
decltype(auto) ReadMemberValue(const typename TMemberPointerTraits<decltype(Member)>::FOwner& Object) {
    if constexpr (std::is_same_v<decltype(Getter), std::nullptr_t>) {
        return Object.*Member;
    } else {
        return std::invoke(Getter, Object);
    }
}

template <auto Member, auto Getter>
FPropertyValue ReadMemberProperty(const void* Container) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;

    return FPropertyValue{ReadMemberValue<Member, Getter>(*static_cast<const FOwner*>(Container))};
}

template <auto Member, auto Getter, auto Setter>
bool WriteMemberProperty(void* Container, const FPropertyValue& Value) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner&>()))>;

    const FValue* Typed{Value.Get<FValue>()};

    if (Typed == nullptr) {
        return false;
    }

    FOwner& Object{*static_cast<FOwner*>(Container)};

    if constexpr (std::is_same_v<decltype(Setter), std::nullptr_t>) {
        Object.*Member = *Typed;
        return true;
    } else {
        return ApplyPropertySetter([&Object, Typed]() {
            return std::invoke(Setter, Object, *Typed);
        });
    }
}

template <auto Member>
void SerializeMemberProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = typename TMemberPointerTraits<decltype(Member)>::FValue;

    FOwner& Object{*static_cast<FOwner*>(Container)};

    if constexpr (requires { FValue::StaticStruct(); }) {
        if (Property.HasAnyFlags(EPropertyFlags::Inline)) {
            SerializeScriptStructProperties(FValue::StaticStruct(), &(Object.*Member), Archive, true);
            return;
        }
    }

    FValue Value{Object.*Member};

    SerializeNativeValue(Archive, Property.GetSerializationName(), Value);

    if (Archive.IsLoading()) {
        Object.*Member = std::move(Value);
    }
}

template <auto Member>
void VisitMemberProperty(const void* Container, const std::function<void(UObject*)>& Visitor) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = typename TMemberPointerTraits<decltype(Member)>::FValue;

    const FOwner& Object{*static_cast<const FOwner*>(Container)};

    VisitNativeReferences<FValue>(&(Object.*Member), Visitor);
}

template <auto Member, auto Getter, auto Setter>
std::unique_ptr<FProperty> MakeProperty(FReflectionBuilder& Owner, FName Name, EPropertyFlags Flags, FProperty::FSerializer Serializer) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner&>()))>;

    return std::make_unique<FProperty>(Name, Owner.GetOwner(), GetNativeType<FValue>(), Flags, &ReadMemberProperty<Member, Getter>, &WriteMemberProperty<Member, Getter, Setter>, Serializer, &VisitMemberProperty<Member>);
}

template <auto Getter, typename TResult>
TResult ReadPropertyMetadata(const void* Container) {
    using FOwner = typename TFunctionPointerTraits<decltype(Getter)>::FOwner;

    return static_cast<TResult>(std::invoke(Getter, *static_cast<const FOwner*>(Container)));
}

template <auto Method>
bool FilterPropertyObject(const void* Container, UObject* Candidate) {
    using FTraits = TFunctionPointerTraits<decltype(Method)>;
    using FOwner = typename FTraits::FOwner;
    using FObject = std::tuple_element_t<0, typename FTraits::FArguments>;

    return std::invoke(Method, *static_cast<const FOwner*>(Container), static_cast<FObject>(Candidate));
}

template <auto Member, auto Getter>
std::unique_ptr<FProperty> MakeReadOnlyProperty(FReflectionBuilder& Owner, FName Name, EPropertyFlags Flags, FProperty::FSerializer Serializer) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner&>()))>;

    return std::make_unique<FProperty>(Name, Owner.GetOwner(), GetNativeType<FValue>(), Flags | EPropertyFlags::ReadOnly, &ReadMemberProperty<Member, Getter>, nullptr, Serializer, &VisitMemberProperty<Member>);
}

template <typename TOption>
void ApplyPropertyOption(FProperty& Property, const TOption& Option) {
    if constexpr (std::is_same_v<std::remove_cvref_t<TOption>, FProperty::FSerializer>) {
        Property.SetSerializer(Option);
    } else {
        Option(Property);
    }
}

template <typename... TOptions>
void RegisterProperty(FReflectionBuilder& Owner, std::unique_ptr<FProperty> Property, const TOptions&... Options) {
    Property->SetEditorMetadata({}, Property->GetDisplayName(), -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 0.1f);
    (ApplyPropertyOption(*Property, Options), ...);
    Owner.AddProperty(std::move(Property));
}

template <auto Method, std::size_t... Indices>
bool InvokeMemberFunction(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result, std::index_sequence<Indices...>) {
    using FTraits = TFunctionPointerTraits<decltype(Method)>;
    using FOwner = typename FTraits::FOwner;
    using FArguments = typename FTraits::FArguments;

    if constexpr (std::is_void_v<typename FTraits::FResult>) {
        std::invoke(Method, static_cast<FOwner&>(Object), *Arguments[Indices].Get<std::remove_cvref_t<std::tuple_element_t<Indices, FArguments>>>()...);
    } else {
        Result = FPropertyValue{std::invoke(Method, static_cast<FOwner&>(Object), *Arguments[Indices].Get<std::remove_cvref_t<std::tuple_element_t<Indices, FArguments>>>()...)};
    }

    return true;
}

template <auto Method>
bool InvokeMemberFunction(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result) {
    using FArguments = typename TFunctionPointerTraits<decltype(Method)>::FArguments;

    return InvokeMemberFunction<Method>(Object, Arguments, Result, std::make_index_sequence<std::tuple_size_v<FArguments>>{});
}

template <auto Method, std::size_t... Indices>
void RegisterFunctionParameters(FReflectionBuilder& Owner, FName Name, std::initializer_list<FName> ParameterNames, std::index_sequence<Indices...>) {
    using FTraits = TFunctionPointerTraits<decltype(Method)>;
    using FArguments = typename FTraits::FArguments;
    using FResult = std::remove_cvref_t<typename FTraits::FResult>;

    const FNativeType* ResultType{};

    if constexpr (!std::is_void_v<FResult>) {
        ResultType = GetNativeType<FResult>();
    }

    Owner.AddFunction(Name, static_cast<FReflectionBuilder::FInvoker>(&InvokeMemberFunction<Method>), ParameterNames, {GetNativeType<std::remove_cvref_t<std::tuple_element_t<Indices, FArguments>>>()...}, ResultType);
}

template <auto Method>
void RegisterFunction(FReflectionBuilder& Owner, FName Name, std::initializer_list<FName> ParameterNames) {
    using FArguments = typename TFunctionPointerTraits<decltype(Method)>::FArguments;

    if (ParameterNames.size() != std::tuple_size_v<FArguments>) {
        ErrorHandler::Report("FReflectionBuilder", "Reflected function parameter names must match its signature", ErrorHandler::EErrorLevel::Critical);
    }

    RegisterFunctionParameters<Method>(Owner, Name, ParameterNames, std::make_index_sequence<std::tuple_size_v<FArguments>>{});
}

template <typename T, std::size_t Line>
void RegisterReflectionLine(FReflectionBuilder& Builder) {
    if constexpr (requires { RegisterReflectionMember(TReflectionMember<T, Line>{}, Builder); }) {
        RegisterReflectionMember(TReflectionMember<T, Line>{}, Builder);
    }
}

template <typename T, std::size_t Begin, std::size_t... Indices>
void RegisterReflectionLines(FReflectionBuilder& Builder, std::index_sequence<Indices...>) {
    (RegisterReflectionLine<T, Begin + Indices>(Builder), ...);
}

template <typename T, std::size_t Begin, std::size_t End>
void RegisterReflectionMembers(FReflectionBuilder& Builder) {
    RegisterReflectionLines<T, Begin>(Builder, std::make_index_sequence<End - Begin>{});
}

template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata& Information) {
    FReflectionBuilder Builder{Information};

    if constexpr (requires { RegisterReflectionMember(TReflectionMember<T, 0>{}, Builder); }) {
        RegisterReflectionMember(TReflectionMember<T, 0>{}, Builder);
    }
}
