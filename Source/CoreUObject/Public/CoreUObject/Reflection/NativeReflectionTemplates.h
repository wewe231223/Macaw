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
        } else if constexpr (std::is_same_v<T, CoreUObject::FObjectReference>) {
            return CoreUObject::EPropertyType::ObjectReference;
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
     * @brief 네이티브 값을 프로퍼티 variant의 대응 저장 타입으로 변환합니다.
     */
    template <typename FValue>
    CoreUObject::FPropertyValue ToPropertyValue(const FValue& Value) {
        if constexpr (std::is_enum_v<FValue>) {
            return static_cast<Core::Int32>(Value);
        } else if constexpr (std::is_same_v<FValue, Core::Uint8>) {
            return static_cast<Core::Uint32>(Value);
        } else {
            return CoreUObject::FPropertyValue{Value};
        }
    }

    /**
     * @brief variant 타입과 Uint8 범위를 검사하여 네이티브 값으로 변환합니다.
     */
    template <typename FValue>
    std::optional<FValue> FromPropertyValue(const CoreUObject::FPropertyValue& Value) {
        using FStoredValue = std::conditional_t<std::is_enum_v<FValue>, Core::Int32, std::conditional_t<std::is_same_v<FValue, Core::Uint8>, Core::Uint32, FValue>>;
        const FStoredValue* Typed{std::get_if<FStoredValue>(&Value)};

        if (Typed == nullptr) {
            return std::nullopt;
        }

        if constexpr (std::is_same_v<FValue, Core::Uint8>) {
            if (*Typed > std::numeric_limits<Core::Uint8>::max()) {
                return std::nullopt;
            }
        }

        return static_cast<FValue>(*Typed);
    }

    /**
     * @brief setter 또는 멤버 포인터로 값을 씁니다.
     */
    template <auto Member, auto Setter, typename FValue>
    bool AssignMemberValue(typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner* Object, const FValue& Value) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;

        if constexpr (std::is_same_v<decltype(Setter), std::nullptr_t>) {
            Object->*Member = Value;

            return true;
        } else if constexpr (std::is_void_v<std::invoke_result_t<decltype(Setter), FOwner*, const FValue&>>) {
            std::invoke(Setter, Object, Value);

            return true;
        } else {
            return static_cast<bool>(std::invoke(Setter, Object, Value));
        }
    }

    /**
     * @brief 네이티브 열거형의 값 목록을 조회합니다.
     */
    template <typename T>
    Core::TArray<CoreUObject::FEnumValue> GetNativeEnumValues() {
        Core::TArray<CoreUObject::FEnumValue> Values{};

        if constexpr (std::is_enum_v<T> && requires { GetNativeEnum(CoreUObject::TNativeEnum<T>{}); }) {
            Values = GetNativeEnum(CoreUObject::TNativeEnum<T>{})->GetValues();
        }

        return Values;
    }

    /**
     * @brief 네이티브 멤버를 프로퍼티 variant의 대응 저장 타입으로 변환합니다.
     */
    template <auto Member, auto Getter>
    CoreUObject::FPropertyValue ReadMemberProperty(const void* Container) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        if constexpr (CoreUObject::GetNativePropertyType<FValue>() == CoreUObject::EPropertyType::Opaque) {
            return static_cast<const void*>(std::addressof(CoreUObject::ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container))));
        } else {
            return CoreUObject::ToPropertyValue<FValue>(CoreUObject::ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container)));
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
            const std::optional<FValue> Converted{CoreUObject::FromPropertyValue<FValue>(Value)};

            return Converted.has_value() && CoreUObject::AssignMemberValue<Member, Setter>(static_cast<FOwner*>(Container), *Converted);
        }
    }

    /**
     * @brief 네이티브 멤버의 타입, 플래그와 읽기·쓰기 함수를 등록합니다.
     */
    template <auto Member, auto Getter, auto Setter>
    void RegisterProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        if constexpr (CoreUObject::GetNativePropertyType<FValue>() == CoreUObject::EPropertyType::Opaque) {
            Flags = Flags | Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly;
        }

        Information.AddProperty(std::make_unique<CoreUObject::FProperty>(std::move(Name), CoreUObject::GetNativePropertyType<FValue>(), Flags, &CoreUObject::ReadMemberProperty<Member, Getter>, &CoreUObject::WriteMemberProperty<Member, Getter, Setter>, CoreUObject::GetNativeStruct<FValue>(), CoreUObject::GetNativeEnumValues<FValue>(), nullptr, CoreUObject::GetNativeType<FValue>()));
    }

    /**
     * @brief 읽기 전용 프로퍼티와 로드에 사용하는 별도 쓰기 함수를 등록합니다.
     */
    template <auto Member, auto Getter>
    void RegisterReadOnlyProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        Information.AddProperty(std::make_unique<CoreUObject::FProperty>(std::move(Name), CoreUObject::GetNativePropertyType<FValue>(), Flags | Core::EPropertyFlags::ReadOnly, &CoreUObject::ReadMemberProperty<Member, Getter>, nullptr, CoreUObject::GetNativeStruct<FValue>(), CoreUObject::GetNativeEnumValues<FValue>(), &CoreUObject::WriteMemberProperty<Member, Getter>, CoreUObject::GetNativeType<FValue>()));
    }

    /**
     * @brief 배열 원소 하나를 다루는 프로퍼티를 반환합니다.
     */
    template <typename T>
    const CoreUObject::FProperty* GetArrayElementProperty() {
        if constexpr (CoreUObject::GetNativePropertyType<T>() == CoreUObject::EPropertyType::Opaque) {
            static_assert(requires { T::StaticStruct(); }, "Array elements must be property value types or reflected structs");

            static const CoreUObject::FProperty Element{"Element", CoreUObject::EPropertyType::Struct, Core::EPropertyFlags::Serialize, nullptr, nullptr, T::StaticStruct(), {}, nullptr, CoreUObject::GetNativeType<T>()};

            return &Element;
        } else {
            static const CoreUObject::FProperty Element{"Element", CoreUObject::GetNativePropertyType<T>(), Core::EPropertyFlags::Serialize, [](const void* Container) {
                return CoreUObject::ToPropertyValue<T>(*static_cast<const T*>(Container));
            }, [](void* Container, const CoreUObject::FPropertyValue& Value) {
                const std::optional<T> Converted{CoreUObject::FromPropertyValue<T>(Value)};

                if (Converted.has_value()) {
                    *static_cast<T*>(Container) = *Converted;
                }

                return Converted.has_value();
            }, CoreUObject::GetNativeStruct<T>(), CoreUObject::GetNativeEnumValues<T>(), nullptr, CoreUObject::GetNativeType<T>()};

            return &Element;
        }
    }

    /**
     * @brief 배열 타입의 함수 테이블을 반환합니다.
     */
    template <typename FArray>
    const CoreUObject::FArrayOps* GetArrayOps() {
        using T = typename FArray::value_type;
        static_assert(!std::is_same_v<T, bool>, "TArray<bool> has no addressable elements");

        static const CoreUObject::FArrayOps Ops{[]() -> void* {
            return new FArray{};
        }, [](void* Array) {
            delete static_cast<FArray*>(Array);
        }, [](const void* Array) {
            return static_cast<Core::Uint32>(static_cast<const FArray*>(Array)->size());
        }, [](void* Array, Core::Uint32 Count) {
            static_cast<FArray*>(Array)->resize(Count);
        }, [](void* Array, Core::Uint32 Index) -> void* {
            return std::addressof((*static_cast<FArray*>(Array))[Index]);
        }, CoreUObject::GetArrayElementProperty<T>()};

        return &Ops;
    }

    /**
     * @brief 배열의 주소를 불투명 주소 값으로 반환합니다.
     */
    template <auto Member, auto Getter>
    CoreUObject::FPropertyValue ReadArrayProperty(const void* Container) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        static_assert(std::is_lvalue_reference_v<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>, "Array getters must return a const reference");

        return static_cast<const void*>(std::addressof(CoreUObject::ReadMemberValue<Member, Getter>(static_cast<const FOwner*>(Container))));
    }

    /**
     * @brief 같은 타입의 배열 주소를 받아 setter 또는 멤버 포인터로 배열 전체를 씁니다.
     */
    template <auto Member, auto Getter, auto Setter>
    bool WriteArrayProperty(void* Container, const CoreUObject::FPropertyValue& Value) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        const void* const* Address{std::get_if<const void*>(&Value)};

        return Address != nullptr && *Address != nullptr && CoreUObject::AssignMemberValue<Member, Setter>(static_cast<FOwner*>(Container), *static_cast<const FValue*>(*Address));
    }

    /**
     * @brief 네이티브 TArray 멤버의 플래그, 읽기·쓰기 함수와 함수 테이블을 등록합니다.
     */
    template <auto Member, auto Getter, auto Setter>
    void RegisterArrayProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags) {
        using FOwner = typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner;
        using FValue = std::remove_cvref_t<decltype(CoreUObject::ReadMemberValue<Member, Getter>(std::declval<const FOwner*>()))>;

        Information.AddProperty(std::make_unique<CoreUObject::FProperty>(std::move(Name), CoreUObject::EPropertyType::Array, Flags, &CoreUObject::ReadArrayProperty<Member, Getter>, &CoreUObject::WriteArrayProperty<Member, Getter, Setter>, nullptr, Core::TArray<CoreUObject::FEnumValue>{}, nullptr, CoreUObject::GetNativeType<FValue>(), CoreUObject::GetArrayOps<FValue>()));
    }

}
