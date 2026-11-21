/**
 * @file FEditorTabManager.h
 * @brief 탭과 위젯의 등록·지연 생성·표시 및 기본 도킹 배치를 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/FEditorTab.h"
#include "Editor/Widget/FEditorWidget.h"

#include <functional>
#include <memory>

namespace Editor {

    /**
     * @brief 패널의 기본 도킹 배치 영역입니다.
     */
    enum class EEditorDockRegion {
        /**
         * @brief 중앙 콘텐츠 영역입니다.
         */
        Center,

        /**
         * @brief 왼쪽 패널 영역입니다.
         */
        Left,

        /**
         * @brief 아웃라이너 패널 영역입니다.
         */
        Outliner,

        /**
         * @brief 상세 속성 패널 영역입니다.
         */
        Details,

        /**
         * @brief 하단 패널 영역입니다.
         */
        Bottom
    };

    /**
     * @brief 탭과 위젯의 등록·지연 생성·표시 및 기본 도킹 배치를 관리합니다.
     */
    class FEditorTabManager {
    private:
        /**
         * @brief 탭 등록 정보와 생성 콜백·소유 객체·기본 표시 상태를 보관합니다.
         */
        struct FEntry {
            /**
             * @brief 등록 항목 또는 원본 에셋의 식별자입니다.
             */
            Core::FString mId{};

            /**
             * @brief 탭 메뉴에서 표시할 항목 이름입니다.
             */
            Core::FString mLabel{};
            Core::FString mWindowName{};
            bool mClosable{true};

            /**
             * @brief 탭 콘텐츠를 필요할 때 생성하는 콜백입니다.
             */
            std::function<std::unique_ptr<Editor::FEditorWidget>()> mSpawner{};

            /**
             * @brief 관리자가 선택적으로 수명을 소유하는 위젯입니다.
             */
            std::unique_ptr<Editor::FEditorWidget> mOwnedWidget{};

            /**
             * @brief 관리자가 수명을 소유하는 탭입니다.
             */
            std::unique_ptr<Editor::FEditorTab> mOwnedTab{};

            /**
             * @brief 직접 소유하거나 외부에서 등록한 탭의 비소유 주소입니다.
             */
            Editor::FEditorTab* mTab{};

            /**
             * @brief 외부에서 소유하는 탭의 프레임 그리기 콜백입니다.
             */
            std::function<void(float)> mDraw{};

            /**
             * @brief 초기 배치에서 사용할 도킹 영역입니다.
             */
            Editor::EEditorDockRegion mRegion{Editor::EEditorDockRegion::Bottom};

            /**
             * @brief 탭을 패널·주 탭·드로어 중 어느 방식으로 다룰지 나타냅니다.
             */
            Editor::EEditorTabRole mRole{Editor::EEditorTabRole::Panel};

            /**
             * @brief 탭 등록 당시의 기본 표시 상태입니다.
             */
            bool mDefaultVisible{};
        };

    public:
        /**
         * @brief FEditorTabManager의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Owner 연결할 상위 주 탭입니다.
         */
        explicit FEditorTabManager(Editor::FEditorTab* Owner = nullptr);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorTabManager();

    public:
        /**
         * @brief 위젯을 탭으로 등록하고 기본 역할·도킹 영역·표시 상태를 기록합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @param[in] Label 메뉴 또는 컨트롤에 표시할 레이블입니다.
         * @param[in] Widget 연결하거나 등록할 콘텐츠 위젯입니다.
         * @param[in] Region 기본 배치에 사용할 도킹 영역입니다.
         * @param[in] Role 등록할 탭의 패널·주 탭·드로어 역할입니다.
         * @return 같은 식별자가 없고 등록에 성공하면 true입니다.
         */
        bool RegisterWidget(Core::FString Id, Core::FString Label, Editor::FEditorWidget& Widget, Editor::EEditorDockRegion Region = Editor::EEditorDockRegion::Bottom, Editor::EEditorTabRole Role = Editor::EEditorTabRole::Panel, bool Closable = true, Core::FString WindowName = {});

        /**
         * @brief 위젯을 탭으로 등록하고 기본 역할·도킹 영역·표시 상태를 기록합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @param[in] Label 메뉴 또는 컨트롤에 표시할 레이블입니다.
         * @param[in] Widget 연결하거나 등록할 콘텐츠 위젯입니다.
         * @param[in] Region 기본 배치에 사용할 도킹 영역입니다.
         * @return 같은 식별자가 없고 등록에 성공하면 true입니다.
         */
        bool RegisterWidget(Core::FString Id, Core::FString Label, std::unique_ptr<Editor::FEditorWidget> Widget, Editor::EEditorDockRegion Region = Editor::EEditorDockRegion::Bottom, bool Closable = true, Core::FString WindowName = {});

        /**
         * @brief 외부에서 소유하는 탭과 그리기 콜백을 등록합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @param[in] Label 메뉴 또는 컨트롤에 표시할 레이블입니다.
         * @param[in] Tab 편집기를 포함할 호스트 탭 또는 등록할 외부 소유 탭입니다.
         * @param[in] Draw 외부 소유 탭의 프레임 그리기 콜백입니다.
         * @return 식별자가 중복되지 않고 콜백이 유효하면 true입니다.
         */
        bool RegisterTab(Core::FString Id, Core::FString Label, Editor::FEditorTab& Tab, std::function<void(float)> Draw);

        /**
         * @brief 필요할 때 위젯을 생성할 탭 생성 콜백을 등록합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @param[in] Label 메뉴 또는 컨트롤에 표시할 레이블입니다.
         * @param[in] Spawner 필요할 때 콘텐츠 위젯의 소유권을 반환하는 생성 콜백입니다.
         * @param[in] Role 등록할 탭의 패널·주 탭·드로어 역할입니다.
         * @return 식별자가 중복되지 않고 생성 콜백이 유효하면 true입니다.
         */
        bool RegisterTabSpawner(Core::FString Id, Core::FString Label, std::function<std::unique_ptr<Editor::FEditorWidget>()> Spawner, Editor::EEditorTabRole Role = Editor::EEditorTabRole::Panel, bool Closable = true, Core::FString WindowName = {});

        /**
         * @brief 등록된 생성 콜백을 필요할 때 실행해 탭 콘텐츠를 확보합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 탭 콘텐츠의 비소유 주소이며 생성할 수 없으면 nullptr입니다.
         */
        Editor::FEditorWidget* SpawnTab(const Core::FString& Id);

        /**
         * @brief 등록 탭을 필요할 때 생성하고 표시·포커스를 요청합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 호출한 탭의 비소유 주소이며 찾거나 생성하지 못하면 nullptr입니다.
         */
        Editor::FEditorTab* TryInvokeTab(const Core::FString& Id);

        /**
         * @brief 식별자에 해당하는 생성된 탭을 찾습니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 탭의 비소유 주소이며 아직 생성하지 않았거나 등록하지 않았으면 nullptr입니다.
         */
        Editor::FEditorTab* FindTab(const Core::FString& Id) const;

        /**
         * @brief 위젯 또는 탭의 표시 상태를 반환합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @return 표시하도록 설정되어 있으면 true입니다.
         */
        bool IsVisible(const Core::FString& Id) const;

        /**
         * @brief 표시 상태를 변경하고 탭의 닫기 확인과 숨김 통지를 처리합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @param[in] Visible 창·위젯 또는 도킹 공간의 표시 여부입니다.
         */
        void SetVisible(const Core::FString& Id, bool Visible);

        /**
         * @brief 등록한 일반 탭을 그리고 드로어 역할 탭은 별도 그리기에 맡깁니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Draw(float DeltaTime);

        /**
         * @brief 등록된 탭 또는 에디터 명령을 메뉴 항목으로 표시합니다.
         */
        void DrawMenu();

        /**
         * @brief 모든 탭 등록과 관리자가 소유하는 탭·위젯을 해제합니다.
         */
        void Clear();

        /**
         * @brief 지정한 탭의 등록과 관리자가 소유하는 관련 객체를 제거합니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         */
        void UnregisterTab(const Core::FString& Id);

        /**
         * @brief 탭이 도킹할 수 있는 ImGui 창 클래스 식별자를 지정합니다.
         * @param[in] ClassId 도킹 호환성을 구분할 ImGui 창 클래스 식별자입니다.
         */
        void SetDockClass(Uint32 ClassId);

        /**
         * @brief 지정한 기본 도킹 영역에 일반 탭이 등록되어 있는지 검사합니다.
         * @param[in] Region 기본 배치에 사용할 도킹 영역입니다.
         * @return 드로어가 아닌 탭이 해당 영역에 있으면 true입니다.
         */
        bool HasRegion(Editor::EEditorDockRegion Region) const;

        /**
         * @brief 관리하는 탭 또는 내부 창에 입력 포커스가 있는지 검사합니다.
         * @return 해당 편집기 영역에 포커스가 있으면 true입니다.
         */
        bool IsFocused() const;

        /**
         * @brief 등록된 일반 탭을 기본 도킹 영역에 따라 배치합니다.
         * @param[in] Left 왼쪽 도킹 노드 식별자입니다.
         * @param[in] Outliner 아웃라이너 도킹 노드 식별자입니다.
         * @param[in] Details 상세 속성 도킹 노드 식별자입니다.
         * @param[in] Bottom 하단 도킹 노드 식별자입니다.
         * @param[in] Center 중앙 도킹 노드 식별자입니다.
         */
        void ApplyLayout(Uint32 Left, Uint32 Outliner, Uint32 Details, Uint32 Bottom, Uint32 Center = 0);

        /**
         * @brief 등록 시 기록한 기본 탭 표시 상태를 복원합니다.
         */
        void RestoreDefaults();
        void SetDefaultVisible(const Core::FString& Id, bool Visible);

    private:
        /**
         * @brief 탭 식별자와 생성·소유·표시 정보를 보관하는 등록 목록입니다.
         */
        Core::TArray<FEntry> mEntries{};

        /**
         * @brief 도킹 호환성을 제한하는 ImGui 창 클래스 식별자입니다.
         */
        Uint32 mDockClass{};

        /**
         * @brief 탭 호출 시 함께 표시할 상위 주 탭의 선택적 비소유 주소입니다.
         */
        Editor::FEditorTab* mOwner{};
    };

}
