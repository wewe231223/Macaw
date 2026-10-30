/**
 * @file EditorEngine.h
 * @brief 편집 모델, 뷰포트 클라이언트와 UI 호스트를 조합합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Engine.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"

namespace Platform {
    class FWindowsApplication;
}

namespace Engine {
    class FRenderTarget;
}

namespace Editor {

    class FEditorUI;

    /**
     * @brief 엔진의 월드 실행과 UI 독립 편집 컨텍스트, 뷰포트 및 UI 호스트의 수명을 조합합니다.
     * @ingroup Editor
     */
    class UEditorEngine final : public Engine::UEngine {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UEditorEngine, Engine::UEngine);

    public:
        /**
         * @brief UEditorEngine의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        UEditorEngine();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UEditorEngine() override;

    public:
        /**
         * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UEditorEngine, CoreUObject::EClassFlags::NoFactory);

        /**
         * @fn UClass* UEditorEngine::GetClass() const
         * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
         * @return 실제 클래스 메타데이터의 비소유 주소입니다.
         */
        /**
         * @brief UI 없이 엔진과 편집 월드 연결을 초기화합니다.
         * @return 초기화에 성공하면 true입니다.
         */
        bool Initialize();

        /**
         * @brief 렌더러를 연결하고 편집 월드를 준비합니다.
         * @param[in,out] Renderer 월드 장면 리소스를 제공할 렌더러입니다.
         * @return 초기화에 성공하면 true입니다.
         */
        bool Initialize(RenderCore::IRendererModule* Renderer) override;

        /**
         * @brief 엔진, ImGui 백엔드, 뷰포트와 초기 장면을 준비합니다.
         * @param[in,out] Renderer 뷰포트 및 엔진이 참조할 렌더러입니다.
         * @param[in,out] Platform Win32 창과 메시지를 제공하는 플랫폼 객체입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool InitializeEditor(RenderCore::IRendererModule* Renderer, Platform::FWindowsApplication* Platform);

        /**
         * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
         */
        void Shutdown() override;

        /**
         * @brief 입력을 수집하고 패널, 월드 동기화와 에디터 프레임 렌더링을 진행합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        void TickFrame(float DeltaTime);

        /**
         * @brief UI와 뷰포트가 공유하는 편집 컨텍스트를 반환합니다.
         * @return 엔진이 소유한 편집 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* GetEditorContext();

        /**
         * @brief 에디터 엔진이 소유한 PIE 실행 세션의 주소를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        Editor::FPlayInEditorSession* GetPlaySession();

        /**
         * @brief 에디터 뷰포트 클라이언트를 반환합니다.
         * @return 독립 뷰포트가 소유한 클라이언트의 비소유 주소입니다.
         */
        Editor::FEditorViewportClient* GetViewportClient();

        /**
         * @brief 엔진이 소유한 기본 독립 에디터 뷰포트를 반환합니다.
         * @return 기본 독립 뷰포트의 비소유 주소입니다.
         */
        Editor::FEditorViewport* GetEditorViewport();

        /**
         * @brief 에디터 뷰포트 렌더 타깃의 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::FRenderTarget* GetViewportTarget() const;

        /**
         * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool WantsExit() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 명시적인 편집 월드, 선택과 공통 편집 경로입니다.
         */
        Editor::FEditorContext mContext;
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 카메라와 기즈모 상태를 가진 클라이언트 및 컬러 타깃을 소유한 독립 뷰포트입니다.
         */
        Editor::FEditorViewport mViewport;
        ReflectProperty(mViewport, "Viewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 에디터 엔진이 소유한 PIE 실행 세션입니다.
         */
        Editor::FPlayInEditorSession mPlaySession;
        ReflectProperty(mPlaySession, "PlaySession", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 편집 모델을 표시하는 UI 호스트입니다.
         */
        std::unique_ptr<Editor::FEditorUI> mUI{};
        ReflectProperty(mUI, "UI", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
