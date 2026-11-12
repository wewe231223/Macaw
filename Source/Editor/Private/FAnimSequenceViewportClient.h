#pragma once

#include "Core/Math/FRotator.h"
#include "Core/Math/FVector.h"
#include "Engine/FViewportClient.h"

#include <memory>

namespace RenderCore {
    class IRendererModule;
}

namespace Editor {

    class FAnimationPreviewScene;

    class FAnimSequenceViewportClient final : public Engine::FViewportClient {
    public:
        FAnimSequenceViewportClient(RenderCore::IRendererModule* Renderer, Editor::FAnimationPreviewScene* Scene);
        ~FAnimSequenceViewportClient() override;

    public:
        void Draw(Engine::FSceneViewport* Viewport) override;
        void DrawUI(const Core::FVector2& Size);
        void Render();
        void FrameMesh();
        void SetDisplay(bool ShowBones, bool ShowGrid, bool Wireframe);

    private:
        RenderCore::IRendererModule* mRenderer{};
        Editor::FAnimationPreviewScene* mScene{};
        std::unique_ptr<Engine::FSceneViewport> mViewport{};
        Core::FVector mCenter{};
        Core::FRotator mRotation{-20.0f, 45.0f, 0.0f};
        float mRadius{1.0f};
        float mDistance{3.0f};
        bool mShowBones{};
        bool mShowGrid{true};
        bool mWireframe{};
    };

}
