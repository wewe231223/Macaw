/**
 * @file FLevelEditorToolkit.h
 * @brief 레벨 편집기의 뷰포트·탭·드로어와 공유 서비스를 구성하고 구동합니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/FEditorToolkit.h"
#include "FMessageBox.h"
#include "Editor/FEditorTabManager.h"
#include "Editor/FImGuiUIScale.h"
#include "Editor/Widget/FOutlinerWidget.h"
#include "Editor/Widget/FDetailsWidget.h"
#include "Editor/Widget/FConsoleWidget.h"
#include "Editor/Widget/FPlaceActorsWidget.h"
#include "Editor/Widget/FSequencerWidget.h"
#include "Editor/Widget/FAnimGraphWidget.h"
#include "FEditorViewportLayout.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "FAssetEditorManager.h"
#include "FAssetImportController.h"
#include "FContentBrowserWidget.h"
#include "FEditorCommands.h"
#include "FEditorDrawerManager.h"
#include "FEditorSettings.h"
#include "FEditorShell.h"
#include "FEditorWorkspace.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include <memory>

namespace Editor {
    /**
     * @brief 레벨 편집기의 뷰포트·탭·드로어와 공유 서비스를 구성하고 구동합니다.
     */
    class FLevelEditorToolkit final : public Editor::FEditorToolkit {
    public:
        /**
         * @brief FLevelEditorToolkit의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Viewport 레벨 편집기 주 뷰포트의 비소유 주소입니다.
         * @param[in] PlaySession PIE 실행과 입력 상태를 제공하는 세션입니다.
         */
        FLevelEditorToolkit(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FLevelEditorToolkit() override;

    public:
        /**
         * @brief 뷰포트와 편집기 공유 서비스를 만들고 탭·설정·이벤트 연결을 준비합니다.
         * @param[in] Renderer 렌더 대상과 GPU 리소스를 제공하는 렌더러입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         * @param[in] IniPath ImGui 배치 설정 파일 경로입니다.
         * @return 레벨 편집기 초기화에 성공하면 true입니다.
         */
        bool Initialize(RenderCore::IRendererModule* Renderer, Platform::FWindowsApplication* Platform, const Core::FString& IniPath);

        /**
         * @brief 설정을 저장하고 이벤트·탭·공유 서비스와 추가 뷰포트 리소스를 해제합니다.
         */
        void Shutdown();

        /**
         * @brief 변경된 테마·글꼴·UI 배율을 에디터에 적용합니다.
         */
        void ApplyAppearance();

        /**
         * @brief 콘텐츠·설정·드로어 상태를 갱신합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Tick(float DeltaTime) override;

        /**
         * @brief 주 도킹 공간·레벨 탭·에셋 탭·드로어와 설정 UI를 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Draw(float DeltaTime) override;

        /**
         * @brief 다음 UI 프레임에서 편집기 종료 요청을 처리하도록 예약합니다.
         */
        void RequestClose();

        /**
         * @brief 레벨 뷰포트와 별도 PIE 창의 게임 렌더링을 처리합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Render(float DeltaTime);

        /**
         * @brief Win32 파일 드롭의 원본 경로와 대상 폴더를 큐에 기록합니다.
         * @param[in] Handle Win32 파일 드롭 핸들입니다.
         */
        void QueueFileDrop(void* Handle);

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
         * @brief 레벨 편집기의 주요 위젯과 지연 생성 탭을 등록합니다.
         */
        void RegisterTabs();

        /**
         * @brief 명령·가져오기·카탈로그·설정·드로어 간 알림 콜백을 연결합니다.
         */
        void ConnectServices();

        /**
         * @brief 레벨 편집기 액터와 에셋 표시용 아이콘을 준비합니다.
         */
        void InstallEditorIcons();

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
         * @brief 뷰포트 분할·입력·최대화 배치를 관리하는 서비스입니다.
         */
        Editor::FEditorViewportLayout mViewportLayout;

        /**
         * @brief 글꼴·DPI에 맞는 에디터 표시 배율 서비스입니다.
         */
        Editor::FImGuiUIScale mUIScale{};

        /**
         * @brief 레벨 편집기 콘텐츠와 내부 도킹 공간을 담는 최상위 탭입니다.
         */
        Editor::FEditorTab mTopLevelTab;

        /**
         * @brief 위젯·최상위 탭을 등록하고 표시하는 탭 관리자입니다.
         */
        Editor::FEditorTabManager& mTabs;

        /**
         * @brief 액터 배치 위젯입니다.
         */
        Editor::FPlaceActorsWidget mPlaceActorsWidget;

        /**
         * @brief 월드 아웃라이너 위젯입니다.
         */
        Editor::FOutlinerWidget mOutlinerWidget;

        /**
         * @brief 액터 상세 속성 위젯입니다.
         */
        Editor::FDetailsWidget mDetailsWidget;

        /**
         * @brief 콘솔 출력과 명령 입력 위젯입니다.
         */
        Editor::FConsoleWidget mConsoleWidget;

        /**
         * @brief 레벨 시퀀서 위젯입니다.
         */
        Editor::FSequencerWidget mSequencerWidget;

        /**
         * @brief 애니메이션 그래프 디버그 위젯입니다.
         */
        Editor::FAnimGraphWidget mAnimGraphWidget;

        /**
         * @brief 에셋 검색과 선택에 사용하는 공유 카탈로그입니다.
         */
        std::unique_ptr<Editor::FEditorAssetCatalog> mAssetCatalog{};

        /**
         * @brief 에셋 썸네일과 애니메이션 미리 보기 설정의 공유 캐시입니다.
         */
        std::unique_ptr<Editor::FAssetThumbnailCache> mThumbnails{};

        /**
         * @brief 최상위 탭과 레벨 패널의 도킹 배치·포커스를 관리하는 서비스입니다.
         */
        std::unique_ptr<Editor::FEditorWorkspace> mWorkspace{};

        /**
         * @brief 드로어 등록 정보 또는 공유 드로어 관리 서비스입니다.
         */
        std::unique_ptr<Editor::FEditorDrawerManager> mDrawers{};

        /**
         * @brief 레벨·프로젝트·실행 명령을 처리하는 서비스입니다.
         */
        std::unique_ptr<Editor::FEditorCommands> mCommands{};

        Editor::FMessageBox mMessages{};
        bool mCloseRequested{};

        /**
         * @brief 열린 에셋 편집기를 관리하는 공유 서비스입니다.
         */
        std::unique_ptr<Editor::FAssetEditorManager> mAssetEditors{};

        /**
         * @brief 파일 가져오기와 드롭을 처리하는 공유 서비스입니다.
         */
        std::unique_ptr<Editor::FAssetImportController> mImports{};

        /**
         * @brief 탭 관리자가 소유하는 콘텐츠 브라우저의 비소유 주소입니다.
         */
        Editor::FContentBrowserWidget* mBrowser{};

        /**
         * @brief 에디터 설정 표시·저장을 담당하는 서비스입니다.
         */
        std::unique_ptr<Editor::FEditorSettings> mSettings{};

        /**
         * @brief 에디터 메뉴·도구 모음·상태 표시줄을 그리는 서비스입니다.
         */
        std::unique_ptr<Editor::FEditorShell> mShell{};

        /**
         * @brief 초기화 완료 여부입니다.
         */
        bool mInitialized{};
    };
}
