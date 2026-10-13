#pragma once

#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "CoreUObject/Reflection/FProperty.h"
#include <span>
#include <initializer_list>
#include <tuple>

class UStruct;

class FReflectionBuilder {
public:
    using FInvoker = bool (*)(UObject&, std::span<const FPropertyValue>, FPropertyValue&);

public:
    explicit FReflectionBuilder(UClass& Class);
    explicit FReflectionBuilder(UScriptStruct& Struct);

public:
    UStruct* GetOwner() const;
    void AddProperty(std::unique_ptr<FProperty> Property);
    void AddFunction(FName Name, FInvoker Invoker, std::initializer_list<FName> ParameterNames, std::initializer_list<const FNativeType*> ParameterTypes, const FNativeType* ReturnType);

private:
    UStruct* mOwner{};
    UClass* mClass{};
};

template <typename T, std::size_t Line>
struct TReflectionMember {
};

template <typename T, std::size_t Line>
void RegisterReflectionLine(FReflectionBuilder& Builder);

template <typename T, std::size_t Begin, std::size_t... Indices>
void RegisterReflectionLines(FReflectionBuilder& Builder, std::index_sequence<Indices...>);

template <typename T, std::size_t Begin, std::size_t End>
void RegisterReflectionMembers(FReflectionBuilder& Builder);

template <auto Method, std::size_t... Indices>
void RegisterFunctionParameters(FReflectionBuilder& Owner, FName Name, std::initializer_list<FName> ParameterNames, std::index_sequence<Indices...>);

template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata& Information);

template <typename T>
struct TMemberPointerTraits;

template <typename TValue, typename TOwner>
struct TMemberPointerTraits<TValue TOwner::*> {
public:
    using FOwner = TOwner;
    using FValue = TValue;
};

template <typename T>
struct TFunctionPointerTraits;

template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
public:
    using FOwner = TOwner;
    using FResult = TResult;
    using FArguments = std::tuple<TArguments...>;
};

template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...) const> : TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
};

template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...) noexcept> : TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
};

template <typename TResult, typename TOwner, typename... TArguments>
struct TFunctionPointerTraits<TResult (TOwner::*)(TArguments...) const noexcept> : TFunctionPointerTraits<TResult (TOwner::*)(TArguments...)> {
};

template <auto Member, auto Getter = nullptr>
decltype(auto) ReadMemberValue(const typename TMemberPointerTraits<decltype(Member)>::FOwner& Object);

template <auto Member, auto Getter = nullptr>
FPropertyValue ReadMemberProperty(const void* Container);

template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
bool WriteMemberProperty(void* Container, const FPropertyValue& Value);

template <auto Member>
void SerializeMemberProperty(void* Container, FArchive& Archive, const FProperty& Property);

template <auto Member>
void VisitMemberProperty(const void* Container, const std::function<void(UObject*)>& Visitor);

template <auto Getter, typename TResult>
TResult ReadPropertyMetadata(const void* Container);

template <auto Method>
bool FilterPropertyObject(const void* Container, UObject* Candidate);

template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
std::unique_ptr<FProperty> MakeProperty(FReflectionBuilder& Owner, FName Name, EPropertyFlags Flags = EPropertyFlags::None, FProperty::FSerializer Serializer = nullptr);

template <auto Member, auto Getter = nullptr>
std::unique_ptr<FProperty> MakeReadOnlyProperty(FReflectionBuilder& Owner, FName Name, EPropertyFlags Flags = EPropertyFlags::None, FProperty::FSerializer Serializer = nullptr);

template <typename TOption>
void ApplyPropertyOption(FProperty& Property, const TOption& Option);

template <typename... TOptions>
void RegisterProperty(FReflectionBuilder& Owner, std::unique_ptr<FProperty> Property, const TOptions&... Options);

template <auto Method, std::size_t... Indices>
bool InvokeMemberFunction(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result, std::index_sequence<Indices...>);

template <auto Method>
bool InvokeMemberFunction(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result);

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
        throw std::logic_error{"Reflected function parameter names must match its signature"};
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
