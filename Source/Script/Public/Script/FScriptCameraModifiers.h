/**
 * @file FScriptCameraModifiers.h
 * @brief C# 카메라 모디파이어를 카메라 관리자에 추가하는 스크립트 바인딩입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Engine {
    class APlayerCameraManager;
    class UCameraModifier;
}

namespace Script {

    /**
     * @brief C# 카메라 모디파이어를 카메라 관리자에 추가하는 스크립트 바인딩입니다.
     * @details C#에서는 이 함수를 직접 부르지 않고 PlayerCameraManager.AddCameraModifier&lt;T&gt;()를 씁니다.
     * @ingroup Script
     */
    class FScriptCameraModifiers final {
    public:
        /**
         * @brief C# 클래스와 연결한 UScriptCameraModifier를 만들어 카메라 관리자에 추가합니다.
         * @param[in] CameraManager 모디파이어를 추가할 카메라 관리자입니다.
         * @param[in] ClassName 네임스페이스를 포함한 ScriptCameraModifier 파생 클래스 이름입니다.
         * @param[in] Priority 적용 순서이며 작을수록 먼저 적용합니다.
         * @return 추가한 모디파이어이며, 클래스를 찾지 못하거나 추가하지 못하면 nullptr입니다.
         */
        UFUNCTION()
        static Engine::UCameraModifier* AddModifier(Engine::APlayerCameraManager* CameraManager, const Core::FString& ClassName, Core::Int32 Priority);
    };

}
