/**
 * @file FEditorUI.h
 * @brief ImGui 컨텍스트와 Win32·D3D11 백엔드를 초기화하고 레벨 편집기 프레임을 구동합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "FLevelEditorToolkit.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include <memory>

namespace Editor {

    /**
     * @brief ImGui 컨텍스트와 Win32·D3D11 백엔드를 초기화하고 레벨 편집기 프레임을 구동합니다.
     */
    class FEditorUI {
    public:
        /**
         * @brief FEditorUI의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Viewport 레벨 편집기 주 뷰포트의 비소유 주소입니다.
         * @param[in] PlaySession PIE 실행과 입력 상태를 제공하는 세션입니다.
         */
        FEditorUI(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorUI();

    public:
        /**
         * @brief ImGui 컨텍스트·백엔드와 프로젝트별 레벨 툴킷을 초기화합니다.
         * @param[in] Renderer 렌더 대상과 GPU 리소스를 제공하는 렌더러입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         * @return 필수 의존성과 모든 초기화 단계가 유효하면 true입니다.
         */
        bool Initialize(RenderCore::IRendererModule* Renderer, Platform::FWindowsApplication* Platform);

        /**
         * @brief 툴킷과 ImGui 설정·Win32·D3D11 백엔드 및 컨텍스트를 정리합니다.
         */
        void Shutdown();

        /**
         * @brief ImGui 새 프레임을 시작하고 툴킷을 갱신한 뒤 에디터 UI를 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         * @return 초기화된 UI에서 프레임을 시작했으면 true입니다.
         */
        bool BeginFrame(float DeltaTime);

        /**
         * @brief 레벨 뷰포트를 렌더링하고 ImGui 그리기 데이터를 D3D11에 제출합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         * @return 초기화된 UI에서 프레임 제출을 처리했으면 true입니다.
         */
        bool EndFrame(float DeltaTime);

        /**
         * @brief 에디터 종료가 요청되었는지 검사합니다.
         * @return 종료 요청이 확정되었으면 true입니다.
         */
        bool WantsExit() const;

        /**
         * @brief 현재 활성 뷰포트를 반환합니다.
         * @return 활성 뷰포트의 비소유 주소입니다.
         */
        Editor::FEditorViewport* GetActiveViewport() const;

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 주 뷰포트의 비소유 주소 또는 별도 PIE 창의 소유 뷰포트입니다.
         */
        Editor::FEditorViewport* mViewport{};

        /**
         * @brief PIE의 월드·재생·입력 상태를 관리하는 공유 세션입니다.
         */
        Editor::FPlayInEditorSession* mPlaySession{};

        /**
         * @brief Win32 창·입력·파일 선택을 제공하는 플랫폼 애플리케이션입니다.
         */
        Platform::FWindowsApplication* mPlatform{};

        /**
         * @brief 레벨 편집기 기능과 공유 서비스를 소유하는 툴킷입니다.
         */
        std::unique_ptr<Editor::FLevelEditorToolkit> mToolkit{};

        /**
         * @brief ImGui 창·도킹 배치를 저장할 INI 파일 경로입니다.
         */
        Core::FString mIniPath{};

        /**
         * @brief ImGui 컨텍스트 생성 완료 여부입니다.
         */
        bool mContextCreated{};

        /**
         * @brief ImGui Win32 백엔드 초기화 완료 여부입니다.
         */
        bool mWin32Ready{};

        /**
         * @brief ImGui D3D11 백엔드 초기화 완료 여부입니다.
         */
        bool mDxReady{};

        /**
         * @brief 초기화 완료 여부입니다.
         */
        bool mInitialized{};
    };
}
