/**
 * @file NativeReflectionTemplates.h
 * @brief 네이티브 타입 판별과 프로퍼티 어댑터 템플릿의 정의입니다.
 * @ingroup CoreUObject
 */

#pragma once

/**
 * @brief 템플릿 타입의 크기, 정렬과 식별 문자열을 갖는 정적 메타데이터를 반환합니다.
 */
template <typename T>
const FNativeType* GetNativeType() {
    static const FNativeType Type{sizeof(T), alignof(T), __FUNCSIG__};
    return &Type;
}

/**
 * @brief C++ 타입을 지원하는 프로퍼티 저장 타입으로 분류합니다.
 */
template <typename T>
constexpr EPropertyType GetNativePropertyType() {
    if constexpr (std::is_same_v<T, bool>) {
        return EPropertyType::Boolean;
    } else if constexpr (std::is_same_v<T, Int32>) {
        return EPropertyType::Integer;
    } else if constexpr ((std::is_same_v<T, Uint32> || std::is_same_v<T, Uint8>)) {
        return EPropertyType::Unsigned;
    } else if constexpr (std::is_same_v<T, Uint64>) {
        return EPropertyType::Unsigned64;
    } else if constexpr (std::is_same_v<T, float>) {
        return EPropertyType::Float;
    } else if constexpr (std::is_same_v<T, FString>) {
        return EPropertyType::String;
    } else if constexpr (std::is_same_v<T, FGuid>) {
        return EPropertyType::Guid;
    } else if constexpr (std::is_same_v<T, FVector2>) {
        return EPropertyType::Vector2;
    } else if constexpr (std::is_same_v<T, FVector>) {
        return EPropertyType::Vector;
    } else if constexpr (std::is_same_v<T, FVector4>) {
        return EPropertyType::Vector4;
    } else if constexpr (std::is_same_v<T, FQuat>) {
        return EPropertyType::Quat;
    } else if constexpr (std::is_same_v<T, FRotator>) {
        return EPropertyType::Rotator;
    } else if constexpr (std::is_same_v<T, FMatrix>) {
        return EPropertyType::Matrix;
    } else if constexpr (std::is_same_v<T, FTransform>) {
        return EPropertyType::Transform;
    } else if constexpr (std::is_same_v<T, FBox>) {
        return EPropertyType::Box;
    } else if constexpr (std::is_same_v<T, FRay>) {
        return EPropertyType::Ray;
    } else if constexpr (std::is_enum_v<T>) {
        return EPropertyType::Enum;
    } else {
        return EPropertyType::Opaque;
    }
}

/**
 * @brief 지원되는 복합 값 타입의 구조체 메타데이터를 조회합니다.
 */
template <typename T>
const UStruct* GetNativeStruct() {
    if constexpr (!std::is_same_v<T, FGuid> && !std::is_same_v<T, FMatrix> && GetNativePropertyType<T>() != EPropertyType::Opaque && requires { T::StaticStruct(); }) {
        return T::StaticStruct();
    } else {
        return nullptr;
    }
}

/**
 * @brief 네이티브 구조체와 선택적 부모의 메타데이터를 등록합니다.
 */
template <typename T>
UScriptStruct* DefineNativeStruct(const char* Name) {
    const UScriptStruct* Parent{};

    if constexpr (requires { typename T::Super; }) {
        Parent = T::Super::StaticStruct();
    }

    static UScriptStruct* Struct{FReflectionRegistry::DefineStruct(Name, GetNativeType<T>(), &BuildNativeReflection<T, UScriptStruct>, Parent)};

    return Struct;
}

/**
 * @brief 템플릿 특수화의 구조체 메타데이터를 한 번 등록합니다.
 */
template <typename T>
UScriptStruct* DefineTemplateStruct(const char* Name) {
    static UScriptStruct* Struct{FReflectionRegistry::DefineStruct(Name, GetNativeType<T>(), &BuildNativeReflection<T, UScriptStruct>)};
    return Struct;
}

/**
 * @brief 타입이 생성한 멤버 등록 함수를 리플렉션 빌더로 실행합니다.
 */
template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata* Information) {
    FReflectionBuilder Builder{Information};
    T::RegisterReflectionMembers(Builder);
}

/**
 * @brief 지정된 getter 또는 멤버 포인터로 실제 값을 읽습니다.
 */
template <auto Member, auto Getter>
decltype(auto) ReadMemberValue(const typename TMemberPointerTraits<decltype(Member)>::FOwner* Object) {
    if constexpr (std::is_same_v<decltype(Getter), std::nullptr_t>) {
        return Object->*Member;
    } else {
        return std::invoke(Getter, Object);
    }
}

/**
 * @brief 네이티브 멤버를 프로퍼티 variant의 대응 저장 타입으로 변환합니다.
 */
template <auto Member, auto Getter>
FPropertyValue ReadMemberProperty(const void* Container) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

    if constexpr (std::is_enum_v<FValue>) {
        return static_cast<Int32>(ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container)));
    } else if constexpr (std::is_same_v<FValue, Uint8>) {
        return static_cast<Uint32>(ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container)));
    } else if constexpr (GetNativePropertyType<FValue>() == EPropertyType::Opaque) {
        return static_cast<const void*>(std::addressof(ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container))));
    } else {
        return FPropertyValue{ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container))};
    }
}

/**
 * @brief variant 타입과 Uint8 범위를 검사한 뒤 setter 또는 멤버 포인터로 씁니다.
 */
template <auto Member, auto Getter, auto Setter>
bool WriteMemberProperty(void* Container, const FPropertyValue& Value) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

    if constexpr (GetNativePropertyType<FValue>() == EPropertyType::Opaque) {
        return false;
    } else {
        using FStoredValue = std::conditional_t<std::is_enum_v<FValue>, Int32, std::conditional_t<std::is_same_v<FValue, Uint8>, Uint32, FValue>>;
        const FStoredValue* Typed{std::get_if<FStoredValue>(&Value)};

        if (Typed == nullptr) {
            return false;
        }

        if constexpr (std::is_same_v<FValue, Uint8>) {
            if (*Typed > std::numeric_limits<Uint8>::max()) {
                return false;
            }
        }

        FOwner* Object{static_cast<FOwner*>(Container)};
        const FValue Converted{static_cast<FValue>(*Typed)};

        if constexpr (std::is_same_v<decltype(Setter), std::nullptr_t>) {
            Object->*Member = Converted;

            return true;
        } else if constexpr (std::is_void_v<std::invoke_result_t<decltype(Setter), FOwner*, const FValue&>>) {
            std::invoke(Setter, Object, Converted);

            return true;
        } else {
            return static_cast<bool>(std::invoke(Setter, Object, Converted));
        }
    }
}

/**
 * @brief 네이티브 멤버의 타입, 플래그와 읽기·쓰기 함수를 등록합니다.
 */
template <auto Member, auto Getter, auto Setter>
void RegisterProperty(FReflectionBuilder& Information, FString Name, EPropertyFlags Flags) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

    TArray<FEnumValue> Values{};

    if constexpr (std::is_enum_v<FValue> && requires { GetNativeEnum(TNativeEnum<FValue>{}); }) {
        Values = GetNativeEnum(TNativeEnum<FValue>{})->GetValues();
    }

    if constexpr (GetNativePropertyType<FValue>() == EPropertyType::Opaque) {
        Flags = Flags | EPropertyFlags::Transient | EPropertyFlags::ReadOnly;
    }

    Information.AddProperty(std::make_unique<FProperty>(std::move(Name), GetNativePropertyType<FValue>(), Flags, &ReadMemberProperty<Member, Getter>, &WriteMemberProperty<Member, Getter, Setter>, GetNativeStruct<FValue>(), std::move(Values), nullptr, GetNativeType<FValue>()));
}

/**
 * @brief 읽기 전용 프로퍼티와 로드에 사용하는 별도 쓰기 함수를 등록합니다.
 */
template <auto Member, auto Getter>
void RegisterReadOnlyProperty(FReflectionBuilder& Information, FString Name, EPropertyFlags Flags) {
    using FOwner = typename TMemberPointerTraits<decltype(Member)>::FOwner;
    using FValue = std::remove_cvref_t<decltype(ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

    TArray<FEnumValue> Values{};

    if constexpr (std::is_enum_v<FValue> && requires { GetNativeEnum(TNativeEnum<FValue>{}); }) {
        Values = GetNativeEnum(TNativeEnum<FValue>{})->GetValues();
    }

    Information.AddProperty(std::make_unique<FProperty>(std::move(Name), GetNativePropertyType<FValue>(), Flags | EPropertyFlags::ReadOnly, &ReadMemberProperty<Member, Getter>, nullptr, GetNativeStruct<FValue>(), std::move(Values), &WriteMemberProperty<Member, Getter>, GetNativeType<FValue>()));
}
