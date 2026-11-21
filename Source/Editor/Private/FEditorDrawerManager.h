/**
 * @file FEditorDrawerManager.h
 * @brief 하단 드로어의 열기·닫기·애니메이션과 도킹 전환 및 설정을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/FEditorTab.h"
#include "Editor/FEditorTabManager.h"
#include "Editor/FImGuiUIScale.h"
#include "Editor/Panel/FEditorPanel.h"
#include "Editor/Widget/FEditorWidget.h"
#include "FEditorWorkspace.h"
#include "Serialization/JsonArchive.h"
#include <functional>
#include <memory>

namespace Editor {

    /**
     * @brief 하단 드로어의 열기·닫기·애니메이션과 도킹 전환 및 설정을 관리합니다.
     */
    class FEditorDrawerManager {
    private:
        /**
         * @brief 등록된 드로어의 식별자·콘텐츠·작업 버튼과 펼침 높이를 보관합니다.
         */
        struct FDrawer {
            /**
             * @brief 등록 항목 또는 원본 에셋의 식별자입니다.
             */
            Core::FString mId{};

            /**
             * @brief 드로어에 표시할 제목입니다.
             */
            Core::FString mTitle{};

            /**
             * @brief 연결된 자식 콘텐츠 위젯의 비소유 주소입니다.
             */
            Editor::FEditorWidget* mWidget{};

            /**
             * @brief 연결된 탭의 비소유 주소 또는 툴킷이 소유하는 주 탭입니다.
             */
            Editor::FEditorTab* mTab{};

            /**
             * @brief 드로어 작업 버튼과 콘텐츠를 담는 소유 패널입니다.
             */
            std::unique_ptr<Editor::FEditorPanel> mLayout{};

            /**
             * @brief 도킹 상태에서 표시할 드로어 작업 위젯의 비소유 주소입니다.
             */
            Editor::FEditorWidget* mActions{};

            /**
             * @brief 창 또는 드로어의 표시 높이입니다.
             */
            float mHeight{360.0f};
        };

    public:
        /**
         * @brief FEditorDrawerManager의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Tabs 탭 등록과 표시 상태를 관리하는 서비스입니다.
         * @param[in] UIScale DPI·글꼴에 따른 에디터 UI 배율 서비스입니다.
         * @param[in] Workspace 주 탭·레벨 패널의 도킹 배치와 포커스 서비스입니다.
         */
        FEditorDrawerManager(Editor::FEditorTabManager& Tabs, Editor::FImGuiUIScale& UIScale, Editor::FEditorWorkspace& Workspace);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorDrawerManager();

    public:
        /**
         * @brief 등록된 탭을 지정한 높이의 드로어로 연결합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @param[in] Title 표시할 창 또는 드로어 제목입니다.
         * @param[in] Height 대상 영역의 높이이며 단위는 픽셀입니다.
         * @return 드로어 등록에 성공하면 true입니다.
         */
        bool RegisterDrawer(Core::FString Id, Core::FString Title, float Height);

        /**
         * @brief 지정한 드로어의 콘텐츠를 준비하고 열기를 요청합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         */
        void OpenDrawer(const Core::FString& Id);

        /**
         * @brief 지정한 드로어의 열림 상태를 전환합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         */
        void ToggleDrawer(const Core::FString& Id);

        /**
         * @brief 현재 열린 드로어를 닫고 이전 포커스의 복원을 준비합니다.
         */
        void CloseDrawer();

        /**
         * @brief 지정한 드로어가 현재 열려 있는지 검사합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 해당 드로어가 열려 있으면 true입니다.
         */
        bool IsDrawerOpen(const Core::FString& Id) const;

        /**
         * @brief 현재 열려 있는 드로어가 있는지 검사합니다.
         * @return 활성 드로어가 있으면 true입니다.
         */
        bool HasOpenDrawer() const;

        /**
         * @brief 드로어가 일반 패널 탭으로 도킹되었는지 검사합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 해당 탭이 패널 역할이면 true입니다.
         */
        bool IsDocked(const Core::FString& Id) const;

        /**
         * @brief 드로어를 레벨 편집기의 일반 도킹 패널로 전환합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         */
        void DockDrawer(const Core::FString& Id);

        /**
         * @brief 도킹된 패널을 드로어 역할로 되돌립니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         */
        void RestoreDrawer(const Core::FString& Id);

        /**
         * @brief 변경이 발생했을 때 호출할 알림 콜백을 지정합니다.
         * @param[in] Handler 변경 내용을 알릴 콜백입니다.
         */
        void SetChangedHandler(std::function<void()> Handler);

        /**
         * @brief 에디터 설정을 아카이브 모드에 따라 읽거나 씁니다.
         * @param[in,out] Archive 읽기·쓰기 모드를 제공하는 아카이브입니다.
         */
        void SerializeSettings(Serialization::FJsonArchive& Archive);

        /**
         * @brief 예약한 도킹·드로어 복원 작업을 처리하고 변경을 알립니다.
         */
        void Tick();

        /**
         * @brief 열린 드로어의 등장·닫힘과 크기 조절 및 콘텐츠를 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Draw(float DeltaTime);

        /**
         * @brief 프레임 끝에 끌어 놓기가 끝났으면 드래그 때문에 접은 콘텐츠 드로어의 복원 예약을 해제합니다.
         * @note 다른 창의 그리기가 모두 끝난 뒤 프레임마다 호출해야 합니다.
         */
        void FinishDragFrame();

    private:
        /**
         * @brief 식별자에 해당하는 등록 드로어를 찾습니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 드로어 정보의 비소유 주소이며 없으면 nullptr입니다.
         */
        Editor::FEditorDrawerManager::FDrawer* FindDrawer(const Core::FString& Id);

        /**
         * @brief 도킹된 드로어에 드로어 복원 작업 버튼을 표시합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         */
        void DrawDockedActions(const Core::FString& Id);

    private:
        /**
         * @brief 위젯·주 탭을 등록하고 표시하는 탭 관리자입니다.
         */
        Editor::FEditorTabManager& mTabs;

        /**
         * @brief 글꼴·DPI에 맞는 에디터 표시 배율 서비스입니다.
         */
        Editor::FImGuiUIScale& mUIScale;

        /**
         * @brief 주 탭과 레벨 패널의 도킹 배치·포커스를 관리하는 서비스입니다.
         */
        Editor::FEditorWorkspace& mWorkspace;

        /**
         * @brief 등록된 드로어의 콘텐츠·높이·작업 위젯 목록입니다.
         */
        Core::TArray<FDrawer> mDrawers{};

        /**
         * @brief 현재 애니메이션을 표시하는 드로어 식별자입니다.
         */
        Core::FString mActiveId{};

        /**
         * @brief 드로어가 열리기 전에 포커스를 가졌던 창 이름입니다.
         */
        Core::FString mPreviousFocus{};

        /**
         * @brief 도킹 또는 드로어 복원 처리 대기 중인 식별자입니다.
         */
        Core::FString mPendingId{};

        /**
         * @brief 편집 내용 또는 설정 변경을 알리는 콜백입니다.
         */
        std::function<void()> mChanged{};

        /**
         * @brief 드로어 열기·닫기 애니메이션의 현재 진행 값입니다.
         */
        float mProgress{};

        /**
         * @brief 예약한 드로어 작업이 도킹인지 복원인지 나타냅니다.
         */
        bool mPendingDock{};

        /**
         * @brief 끌어 놓기를 시작해 열린 콘텐츠 드로어를 접었는지 나타내며 드래그가 끝나면 드로어를 다시 엽니다.
         */
        bool mContentDrawerCollapsedForDrag{};
    };
}
