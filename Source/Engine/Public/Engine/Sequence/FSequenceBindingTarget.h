/**
 * @file FSequenceBindingTarget.h
 * @brief 시퀀스 바인딩과 씬 객체의 연결에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Guid.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectReference.h"
#include "FSequenceBindingTarget.generated.h"

namespace Engine {

    /**
     * @brief 시퀀스 바인딩과 씬 객체의 연결을 저장합니다.
     * @ingroup Engine
     */
    USTRUCT()
    struct FSequenceBindingTarget {
        GENERATED_BODY()

    public:
        /**
         * @brief 시퀀스 에셋 안의 바인딩 ID입니다.
         */
        UPROPERTY(Serialize, Name = "Binding")
        Core::FGuid mBindingId{};

        /**
         * @brief 연결할 액터 또는 공간 컴포넌트입니다.
         */
        UPROPERTY(Serialize)
        CoreUObject::FObjectReference mObject{};
    };

}
