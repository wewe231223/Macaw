/**
 * @file FWorldHit.h
 * @brief 월드 광선 질의로 찾은 액터, 컴포넌트와 충돌 위치 및 거리입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/TObjectRef.h"
#include "Core/Math/FVector.h"

namespace Engine {

    class AActor;
    class UPrimitiveComponent;

    /**
     * @brief 월드 광선 질의로 찾은 액터, 컴포넌트와 충돌 위치 및 거리입니다.
     * @ingroup Engine
     */
    struct FWorldHit {
    public:
        /**
         * @brief 충돌한 액터 핸들입니다.
         */
        CoreUObject::TObjectRef<Engine::AActor> mActor{};

        /**
         * @brief 충돌한 컴포넌트 핸들입니다.
         */
        CoreUObject::TObjectRef<Engine::UPrimitiveComponent> mComponent{};

        /**
         * @brief 월드 좌표의 충돌 위치입니다.
         */
        Core::FVector mPosition{};

        /**
         * @brief 광선상의 충돌 거리입니다.
         */
        float mDistance{};
    };

}
