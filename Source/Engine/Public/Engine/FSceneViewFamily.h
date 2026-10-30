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
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FSceneViewFamily);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSceneViewFamily);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 연결된 렌더 장면입니다.
         */
        const Engine::FSceneInterface* mScene{};
        ReflectProperty(mScene, "Scene", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그릴 렌더 타깃입니다.
         */
        Engine::FRenderTarget* mTarget{};
        ReflectProperty(mTarget, "Target", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 렌더링에 사용할 카메라 뷰입니다.
         */
        Engine::FSceneView mView{};
        ReflectProperty(mView, "View", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 에디터 보조 도형의 비소유 주소입니다.
         */
        const Engine::FPrimitiveDrawInterface* mEditorElements{};
        ReflectProperty(mEditorElements, "EditorElements", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 장면의 Lit 또는 Wireframe 표시 모드입니다.
         */
        Engine::EViewMode mViewMode{Engine::EViewMode::Lit};
        ReflectProperty(mViewMode, "ViewMode", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 뷰포트별 장면·보조 표시 필터입니다.
         */
        Engine::FEngineShowFlags mShowFlags{};
        ReflectProperty(mShowFlags, "ShowFlags", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
