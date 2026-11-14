/**
 * @file UScriptCameraModifier.cpp
 * @brief C# 카메라 모디파이어 콜백을 네이티브 카메라 평가에 연결합니다.
 * @ingroup Script
 */

#include "Script/UScriptCameraModifier.h"
#include "Script/FScriptRuntime.h"
#include "Engine/APlayerCameraManager.h"

#include <utility>

namespace Script {

    Script::UScriptCameraModifier::UScriptCameraModifier(Core::FString ClassName) : mClassName{std::move(ClassName)} {
    }

    Script::UScriptCameraModifier::~UScriptCameraModifier() {
        if (mInstance != 0) {
            Script::FScriptRuntime::Get()->DestroyModifierInstance(mInstance);
        }
    }

    bool Script::UScriptCameraModifier::AddedToCamera(Engine::APlayerCameraManager* CameraManager) {
        if (mInstance != 0 || mClassName.empty() || !Engine::UCameraModifier::AddedToCamera(CameraManager)) {
            return false;
        }

        // C# AddedToCamera가 GetCameraOwner를 쓸 수 있도록 기반 클래스에서 소유자를 먼저 정합니다.
        mInstance = Script::FScriptRuntime::Get()->CreateModifierInstance(mClassName, this);

        return mInstance != 0;
    }

    bool Script::UScriptCameraModifier::ModifyCamera(float DeltaTime, Engine::FMinimalViewInfo& ViewInfo) {
        return mInstance != 0 && Script::FScriptRuntime::Get()->ModifyCameraInstance(mInstance, DeltaTime, ViewInfo);
    }

}
