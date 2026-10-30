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

namespace Engine {

    Engine::FGameViewportClient::FGameViewportClient(Engine::UEngine* Engine)
		: mEngine{Engine} {
    }

    bool Engine::FGameViewportClient::SetWorld(Engine::FWorldHandle World) {
        const Engine::UWorld* Target{mEngine != nullptr ? mEngine->GetWorld(World) : nullptr};

        if (World.IsValid() && (Target == nullptr || (Target->GetWorldType() != Engine::EWorldType::Game && Target->GetWorldType() != Engine::EWorldType::PIE))) {
            return false;
        }

        mWorld = World;

        return true;
    }

    Engine::FWorldHandle Engine::FGameViewportClient::GetWorldHandle() const {
        return mWorld;
    }

    bool Engine::FGameViewportClient::CalcSceneView(const Core::FVector2& Size, Engine::FSceneView& View) const {
        const Engine::UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};
        const Engine::UCameraComponent* Camera{World != nullptr ? World->GetActiveCamera() : nullptr};
        return Camera != nullptr && Camera->GetCameraView(Size, View);
    }

    void Engine::FGameViewportClient::Input(Engine::FSceneViewport*, const Engine::FViewportInput& Input, float DeltaTime) {
        Engine::UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};

        if (World != nullptr) {
            World->Input(Input, DeltaTime);
        }
    }

    void Engine::FGameViewportClient::Draw(Engine::FSceneViewport* Viewport) {
        Engine::UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};
        RenderCore::IRendererModule* Renderer{mEngine != nullptr ? mEngine->GetRenderer() : nullptr};
        Engine::FSceneView View{};

        if (Viewport == nullptr || World == nullptr || World->GetScene() == nullptr || Renderer == nullptr || !CalcSceneView(Viewport->GetSize(), View)) {
            return;
        }

        const Engine::FSceneViewFamily Family{World->GetScene(), Viewport->GetRenderTarget(), View, nullptr};
        Renderer->BeginRenderingViewFamily(Family);
    }

}
