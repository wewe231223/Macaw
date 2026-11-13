/**
 * @file FAnimSequenceViewportClient.h
 * @brief 애니메이션 뷰어의 이미지 표시, 카메라 입력과 오버레이를 연결합니다.
 * @ingroup Editor
 */

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

        /**
         * @brief 뷰어 이미지와 표시 옵션을 그리고 마우스 카메라 입력을 전달합니다.
         * @param Size 출력 또는 표시 영역 크기(픽셀)입니다.
         */
        void DrawUI(const Core::FVector2& Size);

        /**
         * @brief 현재 표시 옵션과 본 오버레이를 반영하여 독립 미리보기 장면을 렌더링합니다.
         */
        void Render();

        /**
         * @brief 현재 미리보기 메시의 경계에 카메라 중심과 거리를 맞춥니다.
         */
        void FrameMesh();

        /**
         * @brief 본·격자·와이어프레임 표시 옵션을 설정합니다.
         * @param ShowBones 본 오버레이 표시 여부입니다.
         * @param ShowGrid 배경 격자 표시 여부입니다.
         * @param Wireframe 와이어프레임 표시 여부입니다.
         */
        void SetDisplay(bool ShowBones, bool ShowGrid, bool Wireframe);

    private:
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 에디터가 소유하는 애니메이션 미리보기 장면의 비소유 포인터입니다.
         */
        Editor::FAnimationPreviewScene* mScene{};
        std::unique_ptr<Engine::FSceneViewport> mViewport{};
        Core::FVector mCenter{};
        Core::FRotator mRotation{-20.0f, 45.0f, 0.0f};
        float mRadius{1.0f};
        float mDistance{3.0f};

        /**
         * @brief 본 오버레이 표시 여부입니다.
         */
        bool mShowBones{};

        /**
         * @brief 미리보기 격자 표시 여부입니다.
         */
        bool mShowGrid{true};

        /**
         * @brief 와이어프레임 표시 여부입니다.
         */
        bool mWireframe{};
    };

}
