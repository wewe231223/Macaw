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
