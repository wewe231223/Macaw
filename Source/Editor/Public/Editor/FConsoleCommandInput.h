/**
 * @file FConsoleCommandInput.h
 * @brief 명령 실행과 자동 완성 후보 및 입력 기록을 제공하는 콘솔 입력 위젯입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FVector2.h"
#include "Editor/FEditorContext.h"

#include <string_view>
#include <imgui.h>

struct ImGuiWindow;

namespace Editor {

    /**
     * @brief 명령 실행과 자동 완성 후보 및 입력 기록을 제공하는 콘솔 입력 위젯입니다.
     */
    class FConsoleCommandInput {
    public:
        /**
         * @brief FConsoleCommandInput의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        explicit FConsoleCommandInput(Editor::FEditorContext* Context);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FConsoleCommandInput();

    public:
        /**
         * @brief 콘솔 입력·자동 완성·명령 기록과 선택적 실행 버튼을 그립니다.
         * @param[in] Id 등록 또는 그리기 항목을 구분할 식별자입니다.
         * @param[in] Width 대상 영역의 너비이며 단위는 픽셀입니다.
         * @param[in] ShowRunButton 공간이 충분할 때 실행 버튼을 표시할지 지정합니다.
         */
        void Draw(const char* Id, float Width, bool ShowRunButton = false);

    private:
        /**
         * @brief 콘솔 입력 영역 위에 자동 완성 후보와 선택 상태를 표시합니다.
         * @param[in] InputId 콘솔 입력란의 ImGui 식별자입니다.
         * @param[in] Parent 자동 완성 팝업이 속한 입력 창입니다.
         * @param[in] Minimum 대상 사각형의 왼쪽 위 화면 좌표입니다.
         * @param[in] Maximum 대상 사각형의 오른쪽 아래 화면 좌표입니다.
         */
        void DrawSuggestions(Uint32 InputId, ImGuiWindow* Parent, const FVector2& Minimum, const FVector2& Maximum);

        /**
         * @brief 입력 문자열에 맞는 콘솔 명령 후보를 다시 조회합니다.
         * @param[in] Input 자동 완성 후보 조회에 사용할 콘솔 입력 문자열입니다.
         */
        void UpdateSuggestions(std::string_view Input);

        /**
         * @brief 위·아래 방향키에 따라 명령 기록과 작성 중 입력을 전환합니다.
         * @param[in,out] Data 조회하거나 수정할 ImGui 텍스트 입력 콜백 상태입니다.
         */
        void BrowseHistory(ImGuiInputTextCallbackData& Data);

        /**
         * @brief 버퍼 용량을 확인한 뒤 ImGui 입력 문자열과 커서 상태를 교체합니다.
         * @param[in,out] Data 조회하거나 수정할 ImGui 텍스트 입력 콜백 상태입니다.
         * @param[in] Text 입력 버퍼에 넣을 문자열입니다.
         * @return 문자열이 버퍼에 들어가 교체했으면 true입니다.
         */
        static bool ReplaceInput(ImGuiInputTextCallbackData& Data, const Core::FString& Text);

        /**
         * @brief 콘솔 입력 콜백에서 자동 완성·기록 탐색·편집 이벤트를 처리합니다.
         * @param[in] Data 조회하거나 수정할 ImGui 텍스트 입력 콜백 상태입니다.
         * @return ImGui 입력 콜백의 처리 결과입니다.
         */
        static int HandleInput(ImGuiInputTextCallbackData* Data);

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief 콘솔 명령 문자열 입력 버퍼입니다.
         */
        Core::TStaticArray<char, 512> mInput{};

        /**
         * @brief 실행한 콘솔 명령의 기록입니다.
         */
        Core::TArray<Core::FString> mHistory{};

        /**
         * @brief 기록 탐색 전에 작성 중이던 콘솔 입력입니다.
         */
        Core::FString mHistoryDraft{};

        /**
         * @brief 현재 탐색 중인 콘솔 명령 기록 위치입니다.
         */
        Index_t mHistoryPosition{};

        /**
         * @brief 현재 입력에 대응하는 콘솔 명령 후보 목록입니다.
         */
        Core::TArray<Core::FString> mSuggestions{};

        /**
         * @brief 선택된 콘솔 자동 완성 후보 인덱스입니다.
         */
        Index_t mSelectedSuggestion{};

        /**
         * @brief 선택한 콘솔 후보가 보이도록 스크롤할지 나타냅니다.
         */
        bool mScrollToSuggestion{};

        /**
         * @brief 선택한 콘솔 후보를 입력에 적용할지 나타냅니다.
         */
        bool mCompletionRequested{};

        /**
         * @brief 콘솔 입력란이 활성 상태인지 나타냅니다.
         */
        bool mInputActive{};

        /**
         * @brief 마우스가 콘솔 자동 완성 후보 영역 위에 있는지 나타냅니다.
         */
        bool mPopupHovered{};
    };
}
