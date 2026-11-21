/**
 * @file FEditorShell.h
 * @brief 주 메뉴·도구 모음·상태 표시줄과 실행 제어를 그립니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/FConsoleCommandInput.h"
#include "Editor/Widget/FEditorButtonWidget.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorTabManager.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FImGuiUIScale.h"
#include "Editor/FPlayInEditorSession.h"
#include "FAssetEditorManager.h"
#include "FAssetImportController.h"
#include "FEditorCommands.h"
#include "FEditorDrawerManager.h"
#include "FEditorSettings.h"
#include "FEditorViewportLayout.h"
#include "FEditorWorkspace.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include <wrl/client.h>
#include <d3d11.h>

namespace Editor {

    /**
     * @brief 주 메뉴·도구 모음·상태 표시줄과 실행 제어를 그립니다.
     */
    class FEditorShell {

    public:
        /**
         * @brief FEditorShell의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Viewport 레벨 편집기 주 뷰포트의 비소유 주소입니다.
         * @param[in] PlaySession PIE 실행과 입력 상태를 제공하는 세션입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         * @param[in] ViewportLayout 뷰포트 분할과 입력을 관리하는 서비스입니다.
         * @param[in] Tabs 탭 등록과 표시 상태를 관리하는 서비스입니다.
         * @param[in] Workspace 주 탭·레벨 패널의 도킹 배치와 포커스 서비스입니다.
         * @param[in] Settings 에디터 설정을 조회·변경하는 서비스입니다.
         * @param[in] Commands 레벨·프로젝트·실행 명령 서비스입니다.
         * @param[in] Imports 에셋 가져오기와 파일 드롭 서비스입니다.
         * @param[in] AssetEditors 에셋 편집기 열기와 수명을 관리하는 서비스입니다.
         * @param[in] Drawers 드로어의 열기·닫기·도킹을 관리하는 서비스입니다.
         * @param[in] UIScale DPI·글꼴에 따른 에디터 UI 배율 서비스입니다.
         */
        FEditorShell(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession, Platform::FWindowsApplication* Platform, Editor::FEditorViewportLayout& ViewportLayout, Editor::FEditorTabManager& Tabs, Editor::FEditorWorkspace& Workspace, Editor::FEditorSettings& Settings, Editor::FEditorCommands& Commands, Editor::FAssetImportController& Imports, Editor::FAssetEditorManager& AssetEditors, Editor::FEditorDrawerManager& Drawers, Editor::FImGuiUIScale& UIScale);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorShell();

    public:
        /**
         * @brief 에디터 또는 프로젝트 브라우저의 브랜드 이미지 리소스를 로드합니다.
         * @param[in] Renderer 렌더 대상과 GPU 리소스를 제공하는 렌더러입니다.
         */
        void LoadBranding(RenderCore::IRendererModule* Renderer);

        /**
         * @brief 등록된 탭 또는 에디터 명령을 메뉴 항목으로 표시합니다.
         */
        void DrawMenu();

        /**
         * @brief 레벨 편집 도구와 실행 제어를 도구 모음에 표시합니다.
         */
        void DrawEditorToolbar();

        /**
         * @brief 하단 상태 정보와 드로어 버튼·콘솔 명령 입력을 표시합니다.
         */
        void DrawStatusBar();

        /**
         * @brief 그림자 아틀라스 미리 보기와 확대 제어를 표시합니다.
         */
        void DrawShadowAtlas();

    private:
        /**
         * @brief 주 창의 최소화·최대화·닫기 버튼을 그립니다.
         */
        void DrawWindowControls();

        /**
         * @brief 레벨에 액터를 추가하는 메뉴를 그립니다.
         */
        void DrawAddMenu();

        /**
         * @brief PIE 시작·정지·일시 정지와 시뮬레이션 제어를 표시합니다.
         */
        void DrawPlayControls();

        /**
         * @brief PIE 시작 위치·입력·별도 창 옵션을 편집합니다.
         */
        void DrawPlayOptions();

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
        Editor::FEditorViewportLayout& mViewportLayout;

        /**
         * @brief 위젯·주 탭을 등록하고 표시하는 탭 관리자입니다.
         */
        Editor::FEditorTabManager& mTabs;

        /**
         * @brief 주 탭과 레벨 패널의 도킹 배치·포커스를 관리하는 서비스입니다.
         */
        Editor::FEditorWorkspace& mWorkspace;

        /**
         * @brief 에디터 설정 표시·저장을 담당하는 서비스입니다.
         */
        Editor::FEditorSettings& mSettings;

        /**
         * @brief 레벨·프로젝트·실행 명령을 처리하는 서비스입니다.
         */
        Editor::FEditorCommands& mCommands;

        /**
         * @brief 파일 가져오기와 드롭을 처리하는 공유 서비스입니다.
         */
        Editor::FAssetImportController& mImports;

        /**
         * @brief 열린 에셋 편집기를 관리하는 공유 서비스입니다.
         */
        Editor::FAssetEditorManager& mAssetEditors;

        /**
         * @brief 드로어 등록 정보 또는 공유 드로어 관리 서비스입니다.
         */
        Editor::FEditorDrawerManager& mDrawers;

        /**
         * @brief 글꼴·DPI에 맞는 에디터 표시 배율 서비스입니다.
         */
        Editor::FImGuiUIScale& mUIScale;

        /**
         * @brief 에디터 브랜드 로고의 GPU 텍스처 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mBrandLogo{};

        /**
         * @brief 상태 표시줄의 콘솔 명령 입력 위젯입니다.
         */
        Editor::FConsoleCommandInput mStatusCommand;

        /**
         * @brief 그림자 아틀라스 미리 보기 확대 배율입니다.
         */
        Editor::FEditorButtonWidget mSaveButton;
        Editor::FEditorButtonWidget mAddButton;
        Editor::FEditorButtonWidget mSequenceButton;
        Editor::FEditorButtonWidget mSettingsButton;
        Editor::FEditorButtonWidget mPlayButton;
        Editor::FEditorButtonWidget mPauseButton;
        Editor::FEditorButtonWidget mStepButton;
        Editor::FEditorButtonWidget mStopButton;
        Editor::FEditorButtonWidget mSimulateButton;
        Editor::FEditorButtonWidget mPlayOptionsButton;
        float mShadowAtlasZoom{1.0f};
    };

}
