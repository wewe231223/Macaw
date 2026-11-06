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
#include "Core/Input.h"
#include "Engine/FSceneViewFamily.h"
#include "RenderCore/IRendererModule.h"

namespace Engine {

    /**
     * @brief 전달받은 설정과 의존성으로 FGameViewportClient의 초기 상태를 구성합니다.
     */
    Engine::FGameViewportClient::FGameViewportClient(Engine::UEngine* Engine)
		: mEngine{Engine} {
    }

    /**
     * @brief 엔진에서 월드 핸들을 검증하고 실행 뷰포트의 대상을 교체합니다.
     */
    bool Engine::FGameViewportClient::SetWorld(Engine::FWorldHandle World) {
        const Engine::UWorld* Target{mEngine != nullptr ? mEngine->GetWorld(World) : nullptr};

        if (World.IsValid() && (Target == nullptr || (Target->GetWorldType() != Engine::EWorldType::Game && Target->GetWorldType() != Engine::EWorldType::PIE))) {
            return false;
        }

        mWorld = World;

        return true;
    }

    /**
     * @brief 이 클라이언트가 그리는 월드의 핸들을 반환합니다.
     */
    Engine::FWorldHandle Engine::FGameViewportClient::GetWorldHandle() const {
        return mWorld;
    }

    /**
     * @brief 실행 월드의 활성 카메라로 뷰포트 크기에 맞는 장면 뷰를 계산합니다.
     */
    bool Engine::FGameViewportClient::CalcSceneView(const Core::FVector2& Size, Engine::FSceneView& View) const {
        const Engine::UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};
        const Engine::UCameraComponent* Camera{World != nullptr ? World->GetActiveCamera() : nullptr};

        return Camera != nullptr && Camera->GetCameraView(Size, View);
    }

    /**
     * @brief 게임 뷰포트가 연결된 월드에 프레임 입력을 전달합니다.
     */
    void Engine::FGameViewportClient::ProcessInput(const Core::FInput& Input, float DeltaTime) {
        Engine::UWorld* World{mEngine != nullptr ? mEngine->GetWorld(mWorld) : nullptr};

        if (World != nullptr) {
            World->Input(Input, DeltaTime);
        }
    }

    /**
     * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
     */
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
