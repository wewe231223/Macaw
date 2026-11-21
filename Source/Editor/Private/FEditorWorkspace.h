/**
 * @file FEditorWorkspace.h
 * @brief 최상위 탭과 레벨 편집기 내부 패널의 도킹 배치·포커스·배치 버전을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorTab.h"
#include "Editor/FEditorTabManager.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FImGuiUIScale.h"
#include <functional>

namespace Editor {

    /**
     * @brief 최상위 탭과 레벨 편집기 내부 패널의 도킹 배치·포커스·배치 버전을 관리합니다.
     */
    class FEditorWorkspace {

    public:
        /**
         * @brief FEditorWorkspace의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] LevelTab 레벨 편집기를 포함하는 최상위 탭입니다.
         * @param[in] UIScale DPI·글꼴에 따른 에디터 UI 배율 서비스입니다.
         */
        FEditorWorkspace(Editor::FEditorContext* Context, Editor::FEditorTab& LevelTab, Editor::FImGuiUIScale& UIScale);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorWorkspace();

    public:
        /**
         * @brief 툴킷 도킹 공간 위에 표시할 도구 모음 콜백을 지정합니다.
         * @param[in] Handler 도킹 공간 위에 도구 모음을 그릴 콜백입니다.
         */
        void SetToolbar(std::function<void()> Handler);

        /**
         * @brief 배치 버전 변경을 알릴 콜백을 지정합니다.
         * @param[in] Handler 도킹 배치 버전 변경을 알릴 콜백입니다.
         */
        void SetLayoutChanged(std::function<void()> Handler);

        /**
         * @brief 저장된 도킹 배치 버전을 반환합니다.
         * @return 현재 배치 버전입니다.
         */
        Uint32 GetLayoutVersion() const;

        /**
         * @brief 저장된 도킹 배치 버전을 갱신합니다.
         * @param[in] Version 보관할 도킹 배치 버전입니다.
         */
        void SetLayoutVersion(Uint32 Version);

        /**
         * @brief 최상위 탭 도킹 공간을 구성하고 이전 배치 버전을 필요에 따라 갱신합니다.
         */
        void BuildLayout();

        /**
         * @brief 다음 최상위 탭 창에 도킹 클래스와 기본 도킹 위치를 지정합니다.
         */
        void BeginTopLevelTab();

        /**
         * @brief 최상위 탭 그리기 뒤 남아 있는 다음 창 설정을 해제합니다.
         */
        void EndTopLevelTab();

        /**
         * @brief 콘텐츠 브라우저를 열고 필요한 레벨 편집기 포커스를 요청합니다.
         */
        void FocusContentBrowser();

        /**
         * @brief 레벨 편집기 최상위 탭을 열고 포커스를 요청합니다.
         */
        void FocusLevelEditor();

        /**
         * @brief 레벨 편집기 최상위 탭 또는 내부 패널에 포커스가 있는지 검사합니다.
         * @return 레벨 편집기 영역에 포커스가 있으면 true입니다.
         */
        bool IsLevelEditorFocused() const;

        /**
         * @brief 지정한 탭을 레벨 편집기 하단 도킹 영역에 배치합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 대상 탭과 도킹 영역을 확보해 배치했으면 true입니다.
         */
        bool DockTab(const Core::FString& Id);

    private:
        /**
         * @brief 레벨 편집기의 좌측·아웃라이너·상세·하단·중앙 패널 영역을 구성합니다.
         * @param[in] DockSpace 배치를 구성할 ImGui 도킹 공간 식별자입니다.
         */
        void BuildLevelLayout(Uint32 DockSpace);

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 레벨 편집기의 최상위 탭 참조입니다.
         */
        Editor::FEditorTab& mLevelTab;

        /**
         * @brief 위젯·최상위 탭을 등록하고 표시하는 탭 관리자입니다.
         */
        Editor::FEditorTabManager& mTabs;

        /**
         * @brief 글꼴·DPI에 맞는 에디터 표시 배율 서비스입니다.
         */
        Editor::FImGuiUIScale& mUIScale;

        /**
         * @brief 저장된 도킹 배치의 버전입니다.
         */
        Uint32 mLayoutVersion{};

        /**
         * @brief 최상위 탭을 배치하는 루트 도킹 공간 식별자입니다.
         */
        Uint32 mRootDock{};

        /**
         * @brief 레벨 편집기 내부 도킹 공간의 식별자입니다.
         */
        Uint32 mLevelDock{};

        /**
         * @brief 레벨 편집기 하단 패널의 도킹 노드 식별자입니다.
         */
        Uint32 mBottomDock{};

        /**
         * @brief 도킹 배치 버전 변경을 알리는 콜백입니다.
         */
        std::function<void()> mLayoutChanged{};
    };

}
