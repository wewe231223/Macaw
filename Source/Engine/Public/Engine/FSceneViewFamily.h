/**
 * @file FSceneViewFamily.h
 * @brief 렌더링할 장면, 출력 타깃, 뷰와 에디터 보조 도형을 묶습니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FSceneView.h"
#include "Engine/EViewMode.h"
#include "Engine/FEngineShowFlags.h"

namespace Engine {

    class FSceneInterface;
    class FRenderTarget;
    class FPrimitiveDrawInterface;

    /**
     * @brief 렌더링할 장면, 출력 타깃, 뷰와 에디터 보조 도형을 묶습니다.
     * @ingroup Engine
     */
    struct FSceneViewFamily {
    public:
        /**
         * @brief 연결된 렌더 장면입니다.
         */
        const Engine::FSceneInterface* mScene{};

        /**
         * @brief 그릴 렌더 타깃입니다.
         */
        Engine::FRenderTarget* mTarget{};

        /**
         * @brief 렌더링에 사용할 카메라 뷰입니다.
         */
        Engine::FSceneView mView{};

        /**
         * @brief 에디터 보조 도형의 비소유 주소입니다.
         */
        const Engine::FPrimitiveDrawInterface* mEditorElements{};

        /**
         * @brief 장면의 Lit 또는 Wireframe 표시 모드입니다.
         */
        Engine::EViewMode mViewMode{Engine::EViewMode::Lit};

        /**
         * @brief 뷰포트별 장면·보조 표시 필터입니다.
         */
        Engine::FEngineShowFlags mShowFlags{};

        /**
         * @brief 에디터 그리드의 보조선 간격이며 0이면 그리드를 그리지 않습니다.
         */
        float mGridSpacing{};

        const Engine::FPrimitiveDrawInterface* mBackgroundOverlay{};
    };

}
