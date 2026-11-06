/**
 * @file FScriptInterop.cpp
 * @brief 네이티브 프로퍼티·스크립트 ABI의 변환과 객체 조회를 구현합니다.
 * @ingroup Script
 */

#include "Script/FScriptInterop.h"
#include "ScriptAbi.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/UObject.h"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <variant>

namespace Script {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        // 썽크가 반환한 값은 C#이 다음 호출 전에 복사합니다.

        /**
         * @brief 스크립트에 전달할 UTF-8 문자열 임시 저장소입니다.
         */
        Core::FString sStoredString{};

        /**
         * @brief 스크립트에 전달할 객체 핸들 배열 임시 저장소입니다.
         */
        Core::TArray<CoreUObject::FObjectHandle> sStoredObjects{};

        /**
         * @brief 실수 배열을 태그가 지정된 스크립트 ABI 값에 저장합니다.
         * @tparam Count 고정 배열의 원소 수입니다.
         * @param[out] Output 변환한 ABI 값을 기록할 객체입니다.
         * @param[in] Tag ABI 값의 타입 태그입니다.
         * @param[in] Values 검사하거나 ABI에 저장할 배열입니다.
         */
        template <std::size_t Count>
        void StoreFloats(Script::FScriptValue& Output, Script::EScriptValueTag Tag, const float (&Values)[Count]) {
            Output.mTag = static_cast<Core::Int32>(Tag);

            std::copy_n(Values, Count, Output.mFloats);
        }

        /**
         * @brief 정수 비트를 태그가 지정된 스크립트 ABI 값에 저장합니다.
         * @param[out] Output 변환한 ABI 값을 기록할 객체입니다.
         * @param[in] Tag ABI 값의 타입 태그입니다.
         * @param[in] Value 검사·변환·저장할 값입니다.
         */
        void StoreInteger(Script::FScriptValue& Output, Script::EScriptValueTag Tag, Core::Uint64 Value) {
            Output.mTag = static_cast<Core::Int32>(Tag);
            Output.mInteger = Value;
        }

        /**
         * @brief 네이티브 프로퍼티를 스크립트 ABI 값으로 변환합니다.
         * @param[in] Value 검사·변환·저장할 값입니다.
         * @return 타입 태그와 변환한 데이터를 담은 ABI 값입니다.
         */
        Script::FScriptValue ToScriptValue(const CoreUObject::FPropertyValue& Value) {
            Script::FScriptValue Output{};

            std::visit([&Output](const auto& Typed) {
                using FTyped = std::remove_cvref_t<decltype(Typed)>;

                if constexpr (std::is_same_v<FTyped, bool>) {
                    StoreInteger(Output, Script::EScriptValueTag::Bool, Typed ? 1 : 0);
                } else if constexpr (std::is_same_v<FTyped, Core::Int32>) {
                    StoreInteger(Output, Script::EScriptValueTag::Int, static_cast<Core::Uint32>(Typed));
                } else if constexpr (std::is_same_v<FTyped, Core::Uint32>) {
                    StoreInteger(Output, Script::EScriptValueTag::Uint, Typed);
                } else if constexpr (std::is_same_v<FTyped, Core::Uint64>) {
                    StoreInteger(Output, Script::EScriptValueTag::Uint64, Typed);
                } else if constexpr (std::is_same_v<FTyped, float>) {
                    const float Values[]{Typed};
                    StoreFloats(Output, Script::EScriptValueTag::Float, Values);
                } else if constexpr (std::is_same_v<FTyped, Core::FString>) {
                    Output.mTag = static_cast<Core::Int32>(Script::EScriptValueTag::String);
                    Output.mText = Script::Abi::StoreString(Typed);
                } else if constexpr (std::is_same_v<FTyped, Core::FVector2>) {
                    const float Values[]{Typed.x, Typed.y};
                    StoreFloats(Output, Script::EScriptValueTag::Vector2, Values);
                } else if constexpr (std::is_same_v<FTyped, Core::FVector>) {
                    const float Values[]{Typed.x, Typed.y, Typed.z};
                    StoreFloats(Output, Script::EScriptValueTag::Vector3, Values);
                } else if constexpr (std::is_same_v<FTyped, Core::FVector4>) {
                    const float Values[]{Typed.x, Typed.y, Typed.z, Typed.w};
                    StoreFloats(Output, Script::EScriptValueTag::Vector4, Values);
                } else if constexpr (std::is_same_v<FTyped, Core::FQuat>) {
                    const float Values[]{Typed.x, Typed.y, Typed.z, Typed.w};
                    StoreFloats(Output, Script::EScriptValueTag::Quaternion, Values);
                } else if constexpr (std::is_same_v<FTyped, Core::FTransform>) {
                    const Core::FVector& L{Typed.mLocation};
                    const Core::FQuat& R{Typed.mRotation};
                    const Core::FVector& S{Typed.mScale};
                    const float Values[]{L.x, L.y, L.z, R.x, R.y, R.z, R.w, S.x, S.y, S.z};
                    StoreFloats(Output, Script::EScriptValueTag::Transform, Values);
                } else if constexpr (std::is_same_v<FTyped, CoreUObject::FObjectReference>) {
                    Output.mTag = static_cast<Core::Int32>(Script::EScriptValueTag::Object);
                    Output.mObject = Script::Abi::ToHandle(Typed.Resolve());
                }
            }, Value);

            return Output;
        }

        // 프로퍼티가 실제로 담는 타입과 태그가 같을 때만 값을 바꿉니다.

        /**
         * @brief 스크립트 ABI 값을 네이티브 프로퍼티 값으로 검증·변환합니다.
         * @param[in] Input 변환할 스크립트 ABI 값입니다.
         * @param[in] Value 검사·변환·저장할 값입니다.
         * @return 지원하는 프로퍼티 형식으로 변환했으면 true입니다.
         */
        bool FromScriptValue(const Script::FScriptValue& Input, CoreUObject::FPropertyValue& Value) {
            const auto Tag{static_cast<Script::EScriptValueTag>(Input.mTag)};
            const float* F{Input.mFloats};
            bool Matched{};

            std::visit([&](auto& Typed) {
                using FTyped = std::remove_cvref_t<decltype(Typed)>;
                const auto Match{[&Matched, Tag](Script::EScriptValueTag Expected) {
                    Matched = Tag == Expected;
                    return Matched;
                }};

                if constexpr (std::is_same_v<FTyped, bool>) {
                    if (Match(Script::EScriptValueTag::Bool)) {
                        Typed = Input.mInteger != 0;
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::Int32>) {
                    if (Match(Script::EScriptValueTag::Int)) {
                        Typed = static_cast<Core::Int32>(static_cast<Core::Uint32>(Input.mInteger));
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::Uint32>) {
                    if (Match(Script::EScriptValueTag::Uint)) {
                        Typed = static_cast<Core::Uint32>(Input.mInteger);
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::Uint64>) {
                    if (Match(Script::EScriptValueTag::Uint64)) {
                        Typed = Input.mInteger;
                    }
                } else if constexpr (std::is_same_v<FTyped, float>) {
                    if (Match(Script::EScriptValueTag::Float)) {
                        Typed = F[0];
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::FString>) {
                    if (Match(Script::EScriptValueTag::String) && Input.mText != nullptr) {
                        Typed = Input.mText;
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::FVector2>) {
                    if (Match(Script::EScriptValueTag::Vector2)) {
                        Typed = Core::FVector2{F[0], F[1]};
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::FVector>) {
                    if (Match(Script::EScriptValueTag::Vector3)) {
                        Typed = Core::FVector{F[0], F[1], F[2]};
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::FVector4>) {
                    if (Match(Script::EScriptValueTag::Vector4)) {
                        Typed = Core::FVector4{F[0], F[1], F[2], F[3]};
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::FQuat>) {
                    if (Match(Script::EScriptValueTag::Quaternion)) {
                        Typed = Core::FQuat{F[0], F[1], F[2], F[3]};
                    }
                } else if constexpr (std::is_same_v<FTyped, Core::FTransform>) {
                    if (Match(Script::EScriptValueTag::Transform)) {
                        Typed.mLocation = Core::FVector{F[0], F[1], F[2]};
                        Typed.mRotation = Core::FQuat{F[3], F[4], F[5], F[6]};
                        Typed.mScale = Core::FVector{F[7], F[8], F[9]};
                    }
                } else if constexpr (std::is_same_v<FTyped, CoreUObject::FObjectReference>) {
                    if (Match(Script::EScriptValueTag::Object)) {
                        Typed = CoreUObject::FObjectReference{CoreUObject::FObjectRegistry::Get()->Resolve(Input.mObject)};
                    }
                }
            }, Value);

            return Matched;
        }
    }

    /**
     * @brief 반환 문자열을 다음 호출까지 보관합니다.
     */
    const char* Script::Abi::StoreString(Core::FString Value) {
        sStoredString = std::move(Value);

        return sStoredString.c_str();
    }

    /**
     * @brief 보관 버퍼의 크기를 맞춥니다.
     */
    CoreUObject::FObjectHandle* Script::Abi::ResizeObjectBuffer(std::size_t Count) {
        sStoredObjects.resize(Count);

        return sStoredObjects.data();
    }

    /**
     * @brief 객체의 클래스 이름을 파생 클래스부터 세미콜론으로 이어 반환합니다.
     */
    Core::FString Script::FScriptInterop::GetClassChain(CoreUObject::UObject* Object) {
        Core::FString Chain{};

        if (Object != nullptr) {
            for (const CoreUObject::UClass* Class{Object->GetClass()}; Class != nullptr; Class = Class->GetSuperClass()) {
                Chain += Class->GetName();
                Chain += ';';
            }
        }

        return Chain;
    }

    /**
     * @brief 리플렉션 프로퍼티를 읽습니다.
     */
    Script::FScriptValue Script::FScriptInterop::GetProperty(CoreUObject::UObject* Object, const Core::FString& Name) {
        const CoreUObject::FProperty* Property{Object != nullptr ? Object->GetClass()->FindClassProperty(Name) : nullptr};

        return Property != nullptr && Property->GetArrayOps() == nullptr ? ToScriptValue(Property->ReadValue(Object)) : Script::FScriptValue{};
    }

    /**
     * @brief 리플렉션 프로퍼티를 씁니다.
     */
    bool Script::FScriptInterop::SetProperty(CoreUObject::UObject* Object, const Core::FString& Name, const Script::FScriptValue& Value) {
        const CoreUObject::FProperty* Property{Object != nullptr ? Object->GetClass()->FindClassProperty(Name) : nullptr};

        if (Property == nullptr || Property->GetArrayOps() != nullptr) {
            return false;
        }

        CoreUObject::FPropertyValue Current{Property->ReadValue(Object)};

        return FromScriptValue(Value, Current) && Property->WriteValue(Object, Current);
    }

}
