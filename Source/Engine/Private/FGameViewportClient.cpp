/**
 * @file FGameViewportClient.cpp
 * @brief 실행 월드의 활성 카메라 선택과 입력·뷰포트 갱신에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/UWorld.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FViewportInput.h"
#include "Engine/FSceneViewFamily.h"
#include "RenderCore/IRendererModule.h"

FGameViewportClient::FGameViewportClient(UEngine* Engine)
	: mEngine{Engine} {
}

bool FGameViewportClient::SetWorld(FWorldHandle World) {
    const UWorld* Target{mEngine != nullptr ? mEngine->GetWorld(World) : nullptr};
    if (World.IsValid() && (Target == nullptr || (Target->GetWorldType() != EWorldType::Game && Target->GetWorldType() != EWorldType::PIE))) {
        return false;
    }
    mWorld = World;
    return true;
}

FWorldHandle FGameViewportClient::GetWorldHandle() const {
    return mWorld;
}

bool FGameViewportClient::CalcSceneView(const FVector2& Size, FSceneView& View) const {
    const UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};
    const UCameraComponent* Camera{World != nullptr ? World->GetActiveCamera() : nullptr};
    return Camera != nullptr && Camera->GetCameraView(Size, View);
}

void FGameViewportClient::Input(FSceneViewport*, const FViewportInput& Input, float DeltaTime) {
    UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};
    if (World != nullptr) {
        World->Input(Input, DeltaTime);
    }
}

void FGameViewportClient::Draw(FSceneViewport* Viewport) {
    UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};
    IRendererModule* Renderer{mEngine != nullptr ? mEngine->GetRenderer() : nullptr};
    FSceneView View{};
    if (Viewport == nullptr || World == nullptr || World->GetScene() == nullptr || Renderer == nullptr || !CalcSceneView(Viewport->GetSize(), View)) {
        return;
    }
    const FSceneViewFamily Family{World->GetScene(), Viewport->GetRenderTarget(), View, nullptr};
    Renderer->BeginRenderingViewFamily(Family);
}
