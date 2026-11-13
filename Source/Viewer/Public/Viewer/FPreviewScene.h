/**
 * @file FPreviewScene.h
 * @brief 메인 장면과 독립된 EditorPreview 월드와 렌더 장면의 수명을 소유합니다.
 * @ingroup Viewer
 */

#pragma once

#include "Core/Types.h"

#include <memory>

namespace Asset {
    class FAssetManager;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Engine {
    class FWorldContext;
    class UWorld;
    class FSceneInterface;
}

namespace Viewer {

    /**
     * @brief 메인 장면과 독립된 EditorPreview 월드와 렌더 장면의 수명을 소유합니다.
     * @ingroup Viewer
     */
    class FPreviewScene {
    public:

        /**
         * @brief 렌더러·에셋 서비스를 참조하며 미리보기 월드 생성은 Initialize까지 미룹니다.
         * @param Renderer 외부에서 소유하는 렌더러 서비스입니다.
         * @param Assets 외부에서 소유하는 에셋 관리자입니다.
         */
        FPreviewScene(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FPreviewScene();

    public:

        /**
         * @brief 렌더러·에셋 서비스를 연결하고 기본 조명을 가진 독립 미리보기 월드를 생성합니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 월드를 준비했으면 true이며 장면 생성 실패 시 Error에 원인을 기록합니다.
         */
        bool Initialize(Core::FString& Error);

        /**
         * @brief 미리보기 월드의 프레임 말 변경을 렌더 장면에 반영합니다.
         */
        void Synchronize();

        /**
         * @brief 독립 미리보기 월드를 반환합니다.
         * @return 월드 컨텍스트 소유의 비소유 포인터이며 초기화 전에는 nullptr입니다.
         */
        Engine::UWorld* GetWorld() const;

        /**
         * @brief 렌더링에 사용할 독립 미리보기 장면을 반환합니다.
         * @return 콘텐츠 또는 월드가 소유하는 비소유 장면 포인터이며 없으면 nullptr입니다.
         */
        Engine::FSceneInterface* GetScene() const;

    private:

        /**
         * @brief 외부에서 소유하는 렌더러 서비스입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 외부에서 소유하는 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 독립 EditorPreview 월드의 수명을 소유하는 컨텍스트입니다.
         */
        std::unique_ptr<Engine::FWorldContext> mWorldContext{};
    };
}
