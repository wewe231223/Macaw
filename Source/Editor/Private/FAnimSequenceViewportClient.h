/**
 * @file FAnimSequenceViewportClient.h
 * @brief 애니메이션 뷰어의 이미지 표시, 카메라 입력과 오버레이를 연결합니다.
 * @ingroup Editor
 */

#pragma once

#include "Viewer/FAssetViewer.h"

namespace Viewer {
    class FAnimationViewerContent;
}

namespace Editor {

    /**
     * @brief 애니메이션 뷰어의 이미지 표시, 카메라 입력과 오버레이를 연결합니다.
     * @ingroup Editor
     */
    class FAnimSequenceViewportClient {
    public:

        /**
         * @brief 애니메이션 콘텐츠를 공통 뷰어로 이전하고 뷰포트 표시 상태를 구성합니다.
         * @param Renderer 외부에서 소유하는 렌더러 서비스입니다.
         * @param Scene 소유권을 이전할 애니메이션 콘텐츠입니다.
         */
        FAnimSequenceViewportClient(RenderCore::IRendererModule* Renderer, std::unique_ptr<Viewer::FAnimationViewerContent> Scene);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FAnimSequenceViewportClient();

    public:

        /**
         * @brief 뷰어 이미지와 표시 옵션을 그리고 마우스 카메라 입력을 전달합니다.
         * @param Size 출력 또는 표시 영역 크기(픽셀)입니다.
         */
        void DrawUI(const Core::FVector2& Size);

        /**
         * @brief 현재 표시 옵션과 본 오버레이를 반영하여 공통 뷰어를 렌더링합니다.
         */
        void Render();

        /**
         * @brief 다음 뷰포트 표시에서 현재 메시 경계에 카메라를 맞추도록 요청합니다.
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

        /**
         * @brief 카메라·렌더 타깃·촬영을 담당하는 공통 에셋 뷰어입니다.
         */
        Viewer::FAssetViewer mViewer;

        /**
         * @brief 공통 뷰어가 소유하는 애니메이션 콘텐츠의 비소유 포인터입니다.
         */
        Viewer::FAnimationViewerContent* mScene{};

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

        /**
         * @brief 다음 표시에서 에셋 경계에 카메라를 맞출지 여부입니다.
         */
        bool mNeedsFit{true};
    };
}
