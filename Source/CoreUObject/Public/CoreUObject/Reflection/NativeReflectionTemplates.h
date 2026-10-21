#pragma once

template <typename T>
const FNativeType* GetNativeType() {
    static const FNativeType Type{sizeof(T), alignof(T)};
    return &Type;
}

template <typename T>
EPropertyType GetNativePropertyType() {
    if constexpr (std::is_same_v<T, bool>) {
        return EPropertyType::Boolean;
    } else if constexpr (std::is_same_v<T, Int32>) {
        return EPropertyType::Integer;
    } else if constexpr (std::is_same_v<T, Uint32>) {
        return EPropertyType::Unsigned;
    } else if constexpr (std::is_same_v<T, float>) {
        return EPropertyType::Float;
    } else if constexpr (std::is_same_v<T, FString>) {
        return EPropertyType::String;
    } else if constexpr (std::is_same_v<T, FGuid>) {
        return EPropertyType::Guid;
    } else if constexpr (std::is_same_v<T, FVector>) {
        return EPropertyType::Vector;
    } else if constexpr (std::is_same_v<T, FRotator>) {
        return EPropertyType::Rotator;
    } else if constexpr (std::is_same_v<T, FTransform>) {
        return EPropertyType::Transform;
    } else {
        static_assert(std::is_enum_v<T>);

        return EPropertyType::Enum;
    }
}

template <typename T>
const UStruct* GetNativeStruct() {
    if constexpr (requires { T::StaticStruct(); }) {
        return T::StaticStruct();
    } else {
        return nullptr;
    }
}

template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata& Information) {
    FReflectionBuilder Builder{Information};
    T::RegisterReflectionMembers(Builder);
}

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
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner&>()))>;

    if constexpr (std::is_enum_v<FValue>) {
        return static_cast<Int32>(ReadMemberValue<Member, Getter>(*static_cast<const FOwner*>(Container)));
    } else {
        return FPropertyValue{ReadMemberValue<Member, Getter>(*static_cast<const FOwner*>(Container))};
    }
}

template <auto Member, auto Getter, auto Setter>
bool WriteMemberProperty(void* Container, const FPropertyValue& Value) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner&>()))>;
    using FStoredValue = std::conditional_t<std::is_enum_v<FValue>, Int32, FValue>;

    const FStoredValue* Typed{std::get_if<FStoredValue>(&Value)};

    if (Typed == nullptr) {
        return false;
    }

    FOwner& Object{*static_cast<FOwner*>(Container)};
    const FValue Converted{static_cast<FValue>(*Typed)};

    if constexpr (std::is_same_v<decltype(Setter), std::nullptr_t>) {
        Object.*Member = Converted;

        return true;
    } else if constexpr (std::is_void_v<std::invoke_result_t<decltype(Setter), FOwner&, const FValue&>>) {
        std::invoke(Setter, Object, Converted);

        return true;
    } else {
        return static_cast<bool>(std::invoke(Setter, Object, Converted));
    }
}

template <auto Member, auto Getter, auto Setter>
std::unique_ptr<FProperty> MakeProperty(FReflectionBuilder& Owner, FString Name, EPropertyFlags Flags) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner&>()))>;

    static_cast<void>(Owner);
    TArray<FEnumValue> Values{};

    if constexpr (std::is_enum_v<FValue>) {
        Values = GetNativeEnum(TNativeEnum<FValue>{})->GetValues();
    }

    return std::make_unique<FProperty>(std::move(Name), GetNativePropertyType<FValue>(), Flags, &ReadMemberProperty<Member, Getter>, &WriteMemberProperty<Member, Getter, Setter>, GetNativeStruct<FValue>(), std::move(Values));
}

template <auto Member, auto Getter>
std::unique_ptr<FProperty> MakeReadOnlyProperty(FReflectionBuilder& Owner, FString Name, EPropertyFlags Flags) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner&>()))>;

    static_cast<void>(Owner);
    TArray<FEnumValue> Values{};

    if constexpr (std::is_enum_v<FValue>) {
        Values = GetNativeEnum(TNativeEnum<FValue>{})->GetValues();
    }

    return std::make_unique<FProperty>(std::move(Name), GetNativePropertyType<FValue>(), Flags | EPropertyFlags::ReadOnly, &ReadMemberProperty<Member, Getter>, nullptr, GetNativeStruct<FValue>(), std::move(Values), &WriteMemberProperty<Member, Getter>);
}
