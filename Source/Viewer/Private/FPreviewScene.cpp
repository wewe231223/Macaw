/**
 * @file FPreviewScene.cpp
 * @brief 메인 장면과 독립된 EditorPreview 월드와 렌더 장면의 수명을 소유합니다.
 * @ingroup Viewer
 */

#include "Viewer/FPreviewScene.h"

#include "Engine/FWorldContext.h"
#include "Engine/UWorld.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FLightSceneProxy.h"
#include "RenderCore/IRendererModule.h"

using namespace Viewer;

FPreviewScene::FPreviewScene(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets)
	: mRenderer{Renderer},
	  mAssets{Assets} {
}

FPreviewScene::~FPreviewScene() = default;

bool FPreviewScene::Initialize(Core::FString& Error) {
    if (mWorldContext != nullptr) {
        return true;
    }

    auto Context{std::make_unique<Engine::FWorldContext>(Engine::EWorldType::EditorPreview)};
    Engine::UWorld* World{Context->GetWorld()};
    World->SetAssets(mAssets);

    if (mRenderer != nullptr) {
        auto Scene{mRenderer->AllocateScene()};

        if (Scene == nullptr || !World->AttachScene(std::move(Scene))) {
            Error = "Cannot create the preview scene";

            return false;
        }

        Engine::FLightSceneProxy Ambient{};
        Ambient.mIntensity = 0.5f;
        World->GetScene()->AddLight(Ambient);

        Engine::FLightSceneProxy Directional{};
        Directional.mType = Engine::ELightType::Directional;
        Directional.mDirection = Core::FVector{0.35f, 0.6f, -1.0f}.Normalized();
        Directional.mIntensity = 0.8f;
        World->GetScene()->AddLight(Directional);
    }

    mWorldContext = std::move(Context);

    return true;
}

void FPreviewScene::Synchronize() {
    if (Engine::UWorld* World{GetWorld()}) {
        World->SendAllEndOfFrameUpdates();
    }
}

Engine::UWorld* FPreviewScene::GetWorld() const {
    return mWorldContext != nullptr ? mWorldContext->GetWorld() : nullptr;
}

Engine::FSceneInterface* FPreviewScene::GetScene() const {
    const Engine::UWorld* World{GetWorld()};

    return World != nullptr ? World->GetScene() : nullptr;
}
