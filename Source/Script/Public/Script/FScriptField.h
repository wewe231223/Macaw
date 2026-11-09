/**
 * @file FScriptField.h
 * @brief 씬에 저장되는 C# 스크립트 필드 값의 선언입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "CoreUObject/FObjectReference.h"
#include "FScriptField.generated.h"

namespace Script {

    /**
     * @brief 기본값과 다르게 편집한 C# 필드 하나의 이름, 타입과 값을 문자열로 저장합니다.
     * @note 값이 스스로를 설명하므로 스크립트가 컴파일되지 않아도 씬을 열고 저장할 수 있습니다.
     * @ingroup Script
     */
    USTRUCT()
    struct FScriptField {
        GENERATED_BODY()

    public:
        /**
         * @brief C# 필드 이름입니다.
         */
        UPROPERTY(Serialize)
        Core::FString mName{};

        /**
         * @brief 타입 태그이며 Bool, Int, Float, String, Vector3, Actor 중 하나입니다.
         */
        UPROPERTY(Serialize)
        Core::FString mType{};

        /**
         * @brief 로캘과 무관한 형식으로 변환한 값입니다. Vector3는 공백으로 구분합니다.
         */
        UPROPERTY(Serialize)
        Core::FString mValue{};

        /**
         * @brief Actor 타입 필드가 가리키는 액터입니다. GUID로 저장되어 PIE 복제에서도 재연결됩니다.
         */
        UPROPERTY(Serialize)
        CoreUObject::FObjectReference mObject{};
    };

}
