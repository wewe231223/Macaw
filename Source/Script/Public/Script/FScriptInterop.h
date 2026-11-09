/**
 * @file FScriptInterop.h
 * @brief C# 런타임 내부에서만 쓰는 객체 조회와 범용 프로퍼티 접근 함수입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "CoreUObject/FObjectHandle.h"

namespace CoreUObject {
    class UObject;
}

namespace Script {

    /**
     * @brief 프로퍼티 값의 종류입니다. Macaw.ScriptValueTag와 순서가 같아야 합니다.
     */
    enum class EScriptValueTag : Core::Int32 {
        /**
         * @brief 해당 값이나 대상을 지정하지 않은 상태입니다.
         */
        None,

        /**
         * @brief 불리언 값입니다.
         */
        Bool,

        /**
         * @brief 부호 있는 32비트 정수입니다.
         */
        Int,

        /**
         * @brief 부호 없는 32비트 정수입니다.
         */
        Uint,

        /**
         * @brief 부호 없는 64비트 정수입니다.
         */
        Uint64,

        /**
         * @brief 32비트 실수입니다.
         */
        Float,

        /**
         * @brief UTF-8 문자열 값입니다.
         */
        String,

        /**
         * @brief 실수 성분 두 개의 벡터입니다.
         */
        Vector2,

        /**
         * @brief 실수 성분 세 개의 벡터입니다.
         */
        Vector3,

        /**
         * @brief 실수 성분 네 개의 벡터입니다.
         */
        Vector4,

        /**
         * @brief 회전 쿼터니언입니다.
         */
        Quaternion,

        /**
         * @brief 위치·회전·스케일 변환입니다.
         */
        Transform,

        /**
         * @brief 엔진 객체 핸들입니다.
         */
        Object
    };

    /**
     * @brief 경계를 넘는 프로퍼티 값입니다. Macaw.ScriptValue와 배치가 같아야 합니다.
     */
    struct FScriptValue {
        /**
         * @brief 값의 종류입니다.
         */
        Core::Int32 mTag{};

        /**
         * @brief Float과 벡터 계열 값입니다. Transform은 위치 3, 회전 4, 스케일 3 순서입니다.
         */
        float mFloats[10]{};

        /**
         * @brief Bool과 정수 계열 값입니다.
         */
        Core::Uint64 mInteger{};

        /**
         * @brief String 값이며 UTF-8입니다.
         */
        const char* mText{};

        /**
         * @brief Object 값입니다.
         */
        CoreUObject::FObjectHandle mObject{};
    };

    /**
     * @brief C#의 EngineObject가 내부적으로 호출하는 함수 모음입니다.
     * @ingroup Script
     */
    class FScriptInterop final {
    public:
        /**
         * @brief 객체의 클래스 이름을 파생 클래스부터 세미콜론으로 이어 반환합니다.
         * @param[in] Object 조회할 객체입니다.
         * @return C#이 가장 가까운 래퍼를 고르는 데 쓰는 이름 목록입니다.
         */
        UFUNCTION(Internal)
        static Core::FString GetClassChain(CoreUObject::UObject* Object);

        /**
         * @brief 리플렉션 프로퍼티를 읽습니다.
         * @param[in] Object 읽을 객체입니다.
         * @param[in] Name 프로퍼티 표시 이름입니다.
         * @return 읽은 값이며, 프로퍼티가 없거나 지원하지 않는 타입이면 태그가 None입니다.
         */
        UFUNCTION(Internal)
        static Script::FScriptValue GetProperty(CoreUObject::UObject* Object, const Core::FString& Name);

        /**
         * @brief 리플렉션 프로퍼티를 씁니다.
         * @param[in] Object 쓸 객체입니다.
         * @param[in] Name 프로퍼티 표시 이름입니다.
         * @param[in] Value 값이며, 프로퍼티와 태그가 같아야 합니다.
         * @return 값을 쓰면 true입니다.
         */
        UFUNCTION(Internal)
        static bool SetProperty(CoreUObject::UObject* Object, const Core::FString& Name, const Script::FScriptValue& Value);
    };

}
