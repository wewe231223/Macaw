/**
 * @file FScriptCameraModifiers.cpp
 * @brief 관리 코드에서 카메라 모디파이어와 흔들림을 제어할 네이티브 함수를 제공합니다.
 * @ingroup Script
 */

#include "Script/FScriptCameraModifiers.h"
#include "Script/UScriptCameraModifier.h"
#include "Engine/APlayerCameraManager.h"
#include "CoreUObject/Reflection/NewObject.h"

namespace Script {

    Engine::UCameraModifier* Script::FScriptCameraModifiers::AddModifier(Engine::APlayerCameraManager* CameraManager, const Core::FString& ClassName, Core::Int32 Priority) {
        if (CameraManager == nullptr || ClassName.empty()) {
            return nullptr;
        }

        auto Modifier{CoreUObject::NewObject<Script::UScriptCameraModifier>(ClassName)};
        Modifier->SetName(ClassName);
        Modifier->SetPriority(Priority);

        return CameraManager->AddCameraModifier(std::move(Modifier));
    }

}
