/**
 * @file FEditorTab.h
 * @brief 위젯 또는 툴킷 호스트를 담는 창의 표시·포커스·닫기와 도킹 속성을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/EEditorIcon.h"
#include "Core/Math/FVector2.h"
#include "Editor/Widget/FEditorWidget.h"

#include <functional>
#include <memory>
#include <optional>

namespace Editor {

    class FEditorToolkitHost;

    /**
     * @brief 에디터 창을 일반 패널·최상위 탭·드로어로 구분합니다.
     */
    enum class EEditorTabRole {
        /**
         * @brief 편집기 내부 도킹 영역에 배치하는 일반 패널 탭입니다.
         */
        Panel,

        /**
         * @brief 독립된 내부 도킹 공간을 가질 수 있는 최상위 편집기 탭입니다.
         */
        TopLevel,

        /**
         * @brief 하단에서 임시로 펼쳐지는 드로어 탭입니다.
         */
        Drawer
    };

    /**
     * @brief 위젯 또는 툴킷 호스트를 담는 창의 표시·포커스·닫기와 도킹 속성을 관리합니다.
     */
    class FEditorTab {
    public:
        /**
         * @brief FEditorTab의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Name 표시 제목과 고유 ImGui 식별자를 담는 이름입니다.
         * @param[in] Closable 사용자가 탭을 닫을 수 있는지 지정합니다.
         * @param[in] Role 등록할 탭의 패널·최상위 탭·드로어 역할입니다.
         */
        FEditorTab(Core::FString Name, bool Closable, Editor::EEditorTabRole Role = Editor::EEditorTabRole::Panel);

/**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorTab();

        /**
         * @brief 중복된 수명·연결 상태를 방지하기 위해 복사 생성을 금지합니다.
         */
        FEditorTab(const Editor::FEditorTab&) = delete;

        /**
         * @brief 중복된 수명·연결 상태를 방지하기 위해 대입을 금지합니다.
         */
        Editor::FEditorTab& operator=(const Editor::FEditorTab&) = delete;

    public:
        /**
         * @brief 표시 중인 탭과 연결 위젯 또는 내부 툴킷 도킹 공간을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Draw(float DeltaTime);

        /**
         * @brief 위젯 또는 탭을 표시하고 연결된 탭에 포커스를 요청합니다.
         */
        void Open();

        /**
         * @brief 표시 상태를 변경하고 탭의 닫기 확인과 숨김 통지를 처리합니다.
         * @param[in] Visible 창·위젯 또는 도킹 공간의 표시 여부입니다.
         */
        void SetVisible(bool Visible);

        /**
         * @brief 위젯 또는 탭의 표시 상태를 반환합니다.
         * @return 표시하도록 설정되어 있으면 true입니다.
         */
        bool IsVisible() const;

        /**
         * @brief 관리하는 탭 또는 내부 창에 입력 포커스가 있는지 검사합니다.
         * @return 해당 편집기 영역에 포커스가 있으면 true입니다.
         */
        bool IsFocused() const;

        /**
         * @brief 포커스 요청을 읽고 요청 상태를 해제합니다.
         * @return 처리할 포커스 요청이 있었으면 true입니다.
         */
        bool ConsumeFocusRequest();

        /**
         * @brief 위젯 또는 연결된 탭의 창 이름을 반환합니다.
         * @return 표시 이름과 ImGui 식별자를 포함할 수 있는 이름입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 탭의 창 이름과 식별자 문자열을 교체합니다.
         * @param[in] Name 표시 제목과 고유 ImGui 식별자를 담는 이름입니다.
         */
        void SetName(Core::FString Name);

        /**
         * @brief 탭 아이콘과 색상을 설정하고 아이콘용 제목 여백을 확보합니다.
         * @param[in] Icon 탭에 표시할 에디터 아이콘입니다.
         * @param[in] Color 그리기에 사용할 패킹된 색상입니다.
         */
        void SetIcon(Editor::EEditorIcon Icon, Uint32 Color = 0xffffffffu);

        /**
         * @brief 탭의 패널·최상위 탭·드로어 역할을 변경합니다.
         * @param[in] Role 등록할 탭의 패널·최상위 탭·드로어 역할입니다.
         */
        void SetRole(Editor::EEditorTabRole Role);

        /**
         * @brief 탭의 현재 역할을 반환합니다.
         * @return 패널·최상위 탭·드로어 중 현재 역할입니다.
         */
        Editor::EEditorTabRole GetRole() const;

        /**
         * @brief 기존 위젯 연결을 해제하고 부모 패널이 없는 위젯을 탭 콘텐츠로 연결합니다.
         * @param[in] Widget 연결하거나 등록할 콘텐츠 위젯입니다.
         */
        void SetContent(Editor::FEditorWidget* Widget);

        /**
         * @brief 현재 연결된 탭 콘텐츠를 조회합니다.
         * @return 위젯의 비소유 주소이며 연결하지 않았으면 nullptr입니다.
         */
        Editor::FEditorWidget* GetContent() const;

        /**
         * @brief 최상위 탭 내부의 툴킷 호스트를 조회하거나 생성합니다. 최상위 탭 역할이어야 합니다.
         * @return 탭이 소유하는 호스트의 참조입니다.
         */
        Editor::FEditorToolkitHost& GetToolkitHost();

        /**
         * @brief 탭이 도킹할 수 있는 ImGui 창 클래스 식별자를 지정합니다.
         * @param[in] ClassId 도킹 호환성을 구분할 ImGui 창 클래스 식별자입니다.
         */
        void SetDockClass(Uint32 ClassId);

        /**
         * @brief 탭 창에 추가로 적용할 ImGui 창 플래그를 지정합니다.
         * @param[in] Flags 적용할 ImGui 창 또는 선택 항목 플래그입니다.
         */
        void SetWindowFlags(Uint32 Flags);

        /**
         * @brief 탭 콘텐츠의 창 내부 여백을 지정합니다.
         * @param[in] Padding 탭 콘텐츠의 가로·세로 내부 여백입니다.
         */
        void SetContentPadding(const FVector2& Padding);

        /**
         * @brief 단일 탭일 때 도킹 탭 표시줄을 자동으로 숨길지 지정합니다.
         * @param[in] AutoHide 단일 탭의 표시줄을 자동으로 숨길지 지정합니다.
         */
        void SetAutoHideTabBar(bool AutoHide);

        /**
         * @brief 탭을 숨긴 뒤 호출할 닫힘 알림 콜백을 지정합니다.
         * @param[in] Handler 탭을 숨긴 뒤 호출할 알림 콜백입니다.
         */
        void SetClosedHandler(std::function<void()> Handler);

        /**
         * @brief 탭을 숨기기 전에 닫기를 허용할지 검사할 콜백을 지정합니다.
         * @param[in] Handler 탭을 닫아도 되면 true를 반환할 콜백입니다.
         */
        void SetCanCloseHandler(std::function<bool()> Handler);

    private:
        /**
         * @brief 탭 창을 시작하고 아이콘·여백·등장 불투명도를 적용합니다.
         * @return 창 콘텐츠를 그릴 수 있으면 true이며 End()로 마쳐야 합니다.
         */
        bool Begin();

        /**
         * @brief 탭 창과 스타일 범위를 닫고 창 닫기 요청을 처리합니다.
         */
        void End();

        /**
         * @brief 다음 탭 창의 도킹 클래스와 예약된 포커스 요청을 적용합니다.
         */
        void ApplyFocus();

    private:
        /**
         * @brief 표시 제목과 고유 ImGui 식별자를 담는 창 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 사용자가 탭을 닫을 수 있는지 나타냅니다.
         */
        bool mClosable{};

        /**
         * @brief 탭·위젯 또는 뷰포트 슬롯의 표시 여부입니다.
         */
        bool mVisible{true};

        /**
         * @brief 다음 창 그리기에서 포커스를 요청할지 나타냅니다.
         */
        bool mFocusRequested{};

        /**
         * @brief 창 그리기를 시작하기 직전의 표시 상태입니다.
         */
        bool mOpenBeforeBegin{};

        /**
         * @brief 탭을 패널·최상위 탭·드로어 중 어느 방식으로 다룰지 나타냅니다.
         */
        Editor::EEditorTabRole mRole{};

        /**
         * @brief 탭 제목에 표시할 선택적 아이콘입니다.
         */
        std::optional<Editor::EEditorIcon> mIcon{};

        /**
         * @brief 탭 아이콘에 적용할 패킹된 색상입니다.
         */
        Uint32 mIconColor{0xffffffffu};

        /**
         * @brief 탭을 숨긴 뒤 실행하는 닫힘 알림 콜백입니다.
         */
        std::function<void()> mClosedHandler{};

        /**
         * @brief 탭 닫기 요청의 허용 여부를 확인하는 콜백입니다.
         */
        std::function<bool()> mCanCloseHandler{};

        /**
         * @brief 탭에 연결된 콘텐츠 위젯의 비소유 주소입니다.
         */
        Editor::FEditorWidget* mContent{};
        std::weak_ptr<void> mContentLifetime{};

        /**
         * @brief 최상위 탭 내부 패널과 도킹 공간을 소유하는 선택적 호스트입니다.
         */
        std::unique_ptr<Editor::FEditorToolkitHost> mToolkitHost{};

        /**
         * @brief 도킹 호환성을 제한하는 ImGui 창 클래스 식별자입니다.
         */
        Uint32 mDockClass{};

        /**
         * @brief 탭 창에 추가 적용할 ImGui 창 플래그입니다.
         */
        Uint32 mWindowFlags{};

        /**
         * @brief 탭 콘텐츠에 적용할 선택적 내부 여백입니다.
         */
        std::optional<FVector2> mContentPadding{};

        /**
         * @brief 단일 탭 도킹 영역의 탭 표시줄 자동 숨김 여부입니다.
         */
        bool mAutoHideTabBar{};
    };

}
