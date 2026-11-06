/**
 * @file ScriptAbi.h
 * @brief 생성된 C# 바인딩 썽크가 쓰는 경계 타입과 변환 함수입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/Math.h"
#include "Core/Types.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/FWorldHit.h"
#include "Script/FScriptInterop.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/**
 * @brief 네이티브·관리 경계의 정렬·문자열·객체 표현을 변환합니다.
 */
namespace Script::Abi {

    /**
     * @brief System.Numerics.Vector2와 배치가 같은 값입니다.
     */
    struct FVector2 {
        /**
         * @brief X 성분입니다.
         */
        float x{};

        /**
         * @brief Y 성분입니다.
         */
        float y{};
    };

    /**
     * @brief System.Numerics.Vector3와 배치가 같은 값입니다. 엔진 벡터는 alignas(16)이라 그대로 넘길 수 없습니다.
     */
    struct FVector3 {
        /**
         * @brief X 성분입니다.
         */
        float x{};

        /**
         * @brief Y 성분입니다.
         */
        float y{};

        /**
         * @brief Z 성분입니다.
         */
        float z{};
    };

    /**
     * @brief System.Numerics.Quaternion과 배치가 같은 값입니다.
     */
    struct FQuat {
        /**
         * @brief X 성분입니다.
         */
        float x{};

        /**
         * @brief Y 성분입니다.
         */
        float y{};

        /**
         * @brief Z 성분입니다.
         */
        float z{};

        /**
         * @brief W 성분입니다.
         */
        float w{};
    };

    /**
     * @brief Macaw.Transform과 배치가 같은 값입니다.
     */
    struct FTransform {
        /**
         * @brief 위치입니다.
         */
        Script::Abi::FVector3 mLocation{};

        /**
         * @brief 회전입니다.
         */
        Script::Abi::FQuat mRotation{};

        /**
         * @brief 스케일입니다.
         */
        Script::Abi::FVector3 mScale{};
    };

    /**
     * @brief Macaw.RaycastHit과 배치가 같은 값입니다. 맞은 것이 없으면 액터 핸들이 유효하지 않습니다.
     */
    struct FWorldHit {
        /**
         * @brief 맞은 액터입니다.
         */
        CoreUObject::FObjectHandle mActor{};

        /**
         * @brief 맞은 컴포넌트입니다.
         */
        CoreUObject::FObjectHandle mComponent{};

        /**
         * @brief 맞은 위치입니다.
         */
        Script::Abi::FVector3 mPosition{};

        /**
         * @brief 광선 시작점부터의 거리입니다.
         */
        float mDistance{};
    };

    /**
     * @brief 생성된 함수 테이블과 C# 쪽 생성 결과를 대조할 값입니다.
     */
    struct FBindingTable {
        /**
         * @brief 썽크 함수 주소 배열입니다.
         */
        void* const* mThunks{};

        /**
         * @brief 썽크 개수입니다.
         */
        std::intptr_t mCount{};

        /**
         * @brief 모든 함수 시그니처의 해시입니다.
         */
        std::intptr_t mHash{};
    };

    /**
     * @brief 생성된 함수 테이블을 반환합니다.
     * @return 프로세스 수명 동안 유효한 테이블입니다.
     */
    Script::Abi::FBindingTable GetBindingTable();

    /**
     * @brief 반환 문자열을 다음 호출까지 보관합니다.
     * @param[in] Value 보관할 문자열입니다.
     * @return C#이 곧바로 복사할 UTF-8 문자열입니다.
     */
    const char* StoreString(Core::FString Value);

    /**
     * @brief 보관 버퍼의 크기를 맞춥니다.
     * @param[in] Count 저장할 핸들 개수입니다.
     * @return 쓸 수 있는 핸들 버퍼입니다.
     */
    CoreUObject::FObjectHandle* ResizeObjectBuffer(std::size_t Count);

    /**
     * @brief 핸들을 객체로 바꿉니다. 파괴가 예약된 액터는 없는 것으로 봅니다.
     * @tparam T 기대하는 객체 타입입니다.
     * @param[in] Handle 객체 핸들입니다.
     * @return 살아 있는 객체이며, 없으면 nullptr입니다.
     */
    template <typename T>
    T* Resolve(CoreUObject::FObjectHandle Handle) {
        CoreUObject::UObject* Object{CoreUObject::FObjectRegistry::Get()->Resolve(Handle)};

        if constexpr (std::is_same_v<T, CoreUObject::UObject>) {
            const auto* Actor{CoreUObject::Cast<Engine::AActor>(Object)};

            return Actor != nullptr && Actor->IsPendingDestroy() ? nullptr : Object;
        } else {
            T* Typed{CoreUObject::Cast<T>(Object)};

            if constexpr (std::is_base_of_v<Engine::AActor, T>) {
                if (Typed != nullptr && Typed->IsPendingDestroy()) {
                    return nullptr;
                }
            }

            return Typed;
        }
    }

    /**
     * @brief 객체 인수를 해석합니다. 비어 있는 핸들은 nullptr로 넘기고, 파괴된 객체는 실패로 처리합니다.
     * @tparam T 기대하는 객체 타입입니다.
     * @param[in] Handle 객체 핸들입니다.
     * @param[out] Object 해석한 객체입니다.
     * @return 호출을 계속해도 되면 true입니다.
     */
    template <typename T>
    bool ResolveArgument(CoreUObject::FObjectHandle Handle, T*& Object) {
        if (!Handle.IsValid()) {
            Object = nullptr;

            return true;
        }

        Object = Script::Abi::Resolve<T>(Handle);

        return Object != nullptr;
    }

    /**
     * @brief 객체의 핸들을 얻습니다.
     * @param[in] Object 객체이며 nullptr일 수 있습니다.
     * @return 객체 핸들이며, 없으면 유효하지 않은 핸들입니다.
     */
    inline CoreUObject::FObjectHandle ToHandle(const CoreUObject::UObject* Object) {
        return Object != nullptr ? Object->GetHandle() : CoreUObject::FObjectHandle{};
    }

    /**
     * @brief 객체 목록의 핸들을 다음 호출까지 보관합니다.
     * @tparam T 객체 타입입니다.
     * @param[in] Objects 보관할 객체 목록입니다.
     * @param[out] Count 핸들 개수입니다.
     * @return C#이 곧바로 복사할 핸들 배열입니다.
     */
    template <typename T>
    const CoreUObject::FObjectHandle* StoreObjects(const Core::TArray<T*>& Objects, Core::Int32& Count) {
        CoreUObject::FObjectHandle* Buffer{Script::Abi::ResizeObjectBuffer(Objects.size())};

        for (std::size_t Index{}; Index < Objects.size(); ++Index) {
            Buffer[Index] = Script::Abi::ToHandle(Objects[Index]);
        }

        Count = static_cast<Core::Int32>(Objects.size());

        return Buffer;
    }

    /**
     * @brief 엔진 값을 경계 값으로 바꿉니다.
     * @param[in] Value 엔진 값입니다.
     * @return 경계 값입니다.
     */
    inline Script::Abi::FVector2 ToAbi(const Core::FVector2& Value) {
        return {Value.x, Value.y};
    }

    /**
     * @brief 엔진 값을 경계 값으로 바꿉니다.
     * @param[in] Value 엔진 값입니다.
     * @return 경계 값입니다.
     */
    inline Script::Abi::FVector3 ToAbi(const Core::FVector& Value) {
        return {Value.x, Value.y, Value.z};
    }

    /**
     * @brief 엔진 값을 경계 값으로 바꿉니다.
     * @param[in] Value 엔진 값입니다.
     * @return 경계 값입니다.
     */
    inline Script::Abi::FQuat ToAbi(const Core::FQuat& Value) {
        return {Value.x, Value.y, Value.z, Value.w};
    }

    /**
     * @brief 엔진 값을 경계 값으로 바꿉니다.
     * @param[in] Value 엔진 값입니다.
     * @return 경계 값입니다.
     */
    inline Script::Abi::FTransform ToAbi(const Core::FTransform& Value) {
        return {Script::Abi::ToAbi(Value.mLocation), Script::Abi::ToAbi(Value.mRotation), Script::Abi::ToAbi(Value.mScale)};
    }

    /**
     * @brief 엔진 값을 경계 값으로 바꿉니다.
     * @param[in] Value 엔진 값입니다.
     * @return 경계 값입니다.
     */
    inline Script::Abi::FWorldHit ToAbi(const Engine::FWorldHit& Value) {
        return {Value.mActor, Value.mComponent, Script::Abi::ToAbi(Value.mPosition), Value.mDistance};
    }

    /**
     * @brief 프로퍼티 값은 이미 경계 배치이므로 그대로 넘깁니다.
     * @param[in] Value 프로퍼티 값입니다.
     * @return 입력과 같은 값입니다.
     */
    inline const Script::FScriptValue& ToAbi(const Script::FScriptValue& Value) {
        return Value;
    }

    /**
     * @brief 경계 값을 엔진 값으로 바꿉니다.
     * @param[in] Value 경계 값입니다.
     * @return 엔진 값입니다.
     */
    inline Core::FVector2 FromAbi(const Script::Abi::FVector2& Value) {
        return Core::FVector2{Value.x, Value.y};
    }

    /**
     * @brief 경계 값을 엔진 값으로 바꿉니다.
     * @param[in] Value 경계 값입니다.
     * @return 엔진 값입니다.
     */
    inline Core::FVector FromAbi(const Script::Abi::FVector3& Value) {
        return Core::FVector{Value.x, Value.y, Value.z};
    }

    /**
     * @brief 경계 값을 엔진 값으로 바꿉니다.
     * @param[in] Value 경계 값입니다.
     * @return 엔진 값입니다.
     */
    inline Core::FQuat FromAbi(const Script::Abi::FQuat& Value) {
        return Core::FQuat{Value.x, Value.y, Value.z, Value.w};
    }

    /**
     * @brief 경계 값을 엔진 값으로 바꿉니다.
     * @param[in] Value 경계 값입니다.
     * @return 엔진 값입니다.
     */
    inline Core::FTransform FromAbi(const Script::Abi::FTransform& Value) {
        Core::FTransform Transform{};
        Transform.mLocation = Script::Abi::FromAbi(Value.mLocation);
        Transform.mRotation = Script::Abi::FromAbi(Value.mRotation);
        Transform.mScale = Script::Abi::FromAbi(Value.mScale);

        return Transform;
    }

    /**
     * @brief 프로퍼티 값은 이미 경계 배치이므로 그대로 넘깁니다.
     * @param[in] Value 프로퍼티 값입니다.
     * @return 입력과 같은 값입니다.
     */
    inline const Script::FScriptValue& FromAbi(const Script::FScriptValue& Value) {
        return Value;
    }

}
