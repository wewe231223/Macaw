/**
 * @file NativeReflectionTemplates.h
 * @brief 네이티브 타입 판별과 프로퍼티 어댑터 템플릿의 정의입니다.
 * @ingroup CoreUObject
 */

#pragma once

namespace CoreUObject {

    /**
     * @brief 템플릿 타입의 크기, 정렬과 식별 문자열을 갖는 정적 메타데이터를 반환합니다.
     */
    template <typename T>
    const CoreUObject::FNativeType* GetNativeType() {
        static const CoreUObject::FNativeType Type{sizeof(T), alignof(T), __FUNCSIG__};
        return &Type;
    }

    /**
     * @brief C++ 타입을 지원하는 프로퍼티 저장 타입으로 분류합니다.
     */
    template <typename T>
    constexpr CoreUObject::EPropertyType GetNativePropertyType() {
        if constexpr (std::is_same_v<T, bool>) {
            return CoreUObject::EPropertyType::Boolean;
        } else if constexpr (std::is_same_v<T, Core::Int32>) {
            return CoreUObject::EPropertyType::Integer;
        } else if constexpr ((std::is_same_v<T, Core::Uint32> || std::is_same_v<T, Core::Uint8>)) {
            return CoreUObject::EPropertyType::Unsigned;
        } else if constexpr (std::is_same_v<T, Core::Uint64>) {
            return CoreUObject::EPropertyType::Unsigned64;
        } else if constexpr (std::is_same_v<T, float>) {
            return CoreUObject::EPropertyType::Float;
        } else if constexpr (std::is_same_v<T, Core::FString>) {
            return CoreUObject::EPropertyType::String;
        } else if constexpr (std::is_same_v<T, Core::FGuid>) {
            return CoreUObject::EPropertyType::Guid;
        } else if constexpr (std::is_same_v<T, Core::FVector2>) {
            return CoreUObject::EPropertyType::Vector2;
        } else if constexpr (std::is_same_v<T, Core::FVector>) {
            return CoreUObject::EPropertyType::Vector;
        } else if constexpr (std::is_same_v<T, Core::FVector4>) {
            return CoreUObject::EPropertyType::Vector4;
        } else if constexpr (std::is_same_v<T, Core::FQuat>) {
            return CoreUObject::EPropertyType::Quat;
        } else if constexpr (std::is_same_v<T, Core::FRotator>) {
            return CoreUObject::EPropertyType::Rotator;
        } else if constexpr (std::is_same_v<T, Core::FMatrix>) {
            return CoreUObject::EPropertyType::Matrix;
        } else if constexpr (std::is_same_v<T, Core::FTransform>) {
            return CoreUObject::EPropertyType::Transform;
        } else if constexpr (std::is_same_v<T, Core::FBox>) {
            return CoreUObject::EPropertyType::Box;
        } else if constexpr (std::is_same_v<T, Core::FRay>) {
            return CoreUObject::EPropertyType::Ray;
        } else if constexpr (std::is_enum_v<T>) {
            return CoreUObject::EPropertyType::Enum;
        } else {
            return CoreUObject::EPropertyType::Opaque;
        }
    }

    /**
     * @brief 지원되는 복합 값 타입의 구조체 메타데이터를 조회합니다.
     */
    template <typename T>
    const CoreUObject::UStruct* GetNativeStruct() {
        if constexpr (!std::is_same_v<T, Core::FGuid> && !std::is_same_v<T, Core::FMatrix> && CoreUObject::GetNativePropertyType<T>() != CoreUObject::EPropertyType::Opaque && requires { T::StaticStruct(); }) {
            return T::StaticStruct();
        } else {
            return nullptr;
        }
    }

    /**
     * @brief 네이티브 구조체와 선택적 부모의 메타데이터를 등록합니다.
     */
    template <typename T>
    CoreUObject::UScriptStruct* DefineNativeStruct(const char* Name) {
        const CoreUObject::UScriptStruct* Parent{};

        if constexpr (requires { typename T::Super; }) {
            Parent = T::Super::StaticStruct();
        }

        static CoreUObject::UScriptStruct* Struct{CoreUObject::FReflectionRegistry::DefineStruct(Name, CoreUObject::GetNativeType<T>(), &CoreUObject::BuildNativeReflection<T, CoreUObject::UScriptStruct>, Parent)};

        return Struct;
    }

    /**
     * @brief 템플릿 특수화의 구조체 메타데이터를 한 번 등록합니다.
     */
    template <typename T>
    CoreUObject::UScriptStruct* DefineTemplateStruct(const char* Name) {
        static CoreUObject::UScriptStruct* Struct{CoreUObject::FReflectionRegistry::DefineStruct(Name, CoreUObject::GetNativeType<T>(), &CoreUObject::BuildNativeReflection<T, CoreUObject::UScriptStruct>)};
        return Struct;
    }

    /**
     * @brief 타입이 생성한 멤버 등록 함수를 리플렉션 빌더로 실행합니다.
     */
    template <typename T, typename TMetadata>
    void BuildNativeReflection(TMetadata* Information) {
        CoreUObject::FReflectionBuilder Builder{Information};
        T::RegisterReflectionMembers(Builder);
    }

    /**
     * @brief 지정된 getter 또는 멤버 포인터로 실제 값을 읽습니다.
     */
    template <auto Member, auto Getter>
    decltype(auto) CoreUObject::ReadMemberValue(const typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner* Object) {
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
    CoreUObject::FPropertyValue ReadMemberProperty(const void* Container) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        if constexpr (std::is_enum_v<FValue>) {
            return static_cast<Core::Int32>(CoreUObject::ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container)));
        } else if constexpr (std::is_same_v<FValue, Core::Uint8>) {
            return static_cast<Core::Uint32>(CoreUObject::ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container)));
        } else if constexpr (CoreUObject::GetNativePropertyType<FValue>() == CoreUObject::EPropertyType::Opaque) {
            return static_cast<const void*>(std::addressof(CoreUObject::ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container))));
        } else {
            return CoreUObject::FPropertyValue{CoreUObject::ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container))};
        }
    }

    /**
     * @brief variant 타입과 Uint8 범위를 검사한 뒤 setter 또는 멤버 포인터로 씁니다.
     */
    template <auto Member, auto Getter, auto Setter>
    bool WriteMemberProperty(void* Container, const CoreUObject::FPropertyValue& Value) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        if constexpr (CoreUObject::GetNativePropertyType<FValue>() == CoreUObject::EPropertyType::Opaque) {
            return false;
        } else {
            using FStoredValue = std::conditional_t<std::is_enum_v<FValue>, Core::Int32, std::conditional_t<std::is_same_v<FValue, Core::Uint8>, Core::Uint32, FValue>>;
            const FStoredValue* Typed{std::get_if<FStoredValue>(&Value)};

            if (Typed == nullptr) {
                return false;
            }

            if constexpr (std::is_same_v<FValue, Core::Uint8>) {
                if (*Typed > std::numeric_limits<Core::Uint8>::max()) {
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
    void RegisterProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        Core::TArray<CoreUObject::FEnumValue> Values{};

        if constexpr (std::is_enum_v<FValue> && requires { GetNativeEnum(CoreUObject::TNativeEnum<FValue>{}); }) {
            Values = GetNativeEnum(CoreUObject::TNativeEnum<FValue>{})->GetValues();
        }

        if constexpr (CoreUObject::GetNativePropertyType<FValue>() == CoreUObject::EPropertyType::Opaque) {
            Flags = Flags | Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly;
        }

        Information.AddProperty(std::make_unique<CoreUObject::FProperty>(std::move(Name), CoreUObject::GetNativePropertyType<FValue>(), Flags, &CoreUObject::ReadMemberProperty<Member, Getter>, &CoreUObject::WriteMemberProperty<Member, Getter, Setter>, CoreUObject::GetNativeStruct<FValue>(), std::move(Values), nullptr, CoreUObject::GetNativeType<FValue>()));
    }

    /**
     * @brief 읽기 전용 프로퍼티와 로드에 사용하는 별도 쓰기 함수를 등록합니다.
     */
    template <auto Member, auto Getter>
    void RegisterReadOnlyProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        Core::TArray<CoreUObject::FEnumValue> Values{};

        if constexpr (std::is_enum_v<FValue> && requires { GetNativeEnum(CoreUObject::TNativeEnum<FValue>{}); }) {
            Values = GetNativeEnum(CoreUObject::TNativeEnum<FValue>{})->GetValues();
        }

        Information.AddProperty(std::make_unique<CoreUObject::FProperty>(std::move(Name), CoreUObject::GetNativePropertyType<FValue>(), Flags | Core::EPropertyFlags::ReadOnly, &CoreUObject::ReadMemberProperty<Member, Getter>, nullptr, CoreUObject::GetNativeStruct<FValue>(), std::move(Values), &CoreUObject::WriteMemberProperty<Member, Getter>, CoreUObject::GetNativeType<FValue>()));
    }

}
