/**
 * @file FConsolePanel.h
 * @brief 콘솔 로그와 명령 입력을 표시하는 에디터 패널입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"

#include <array>
#include <memory>
#include <string_view>

struct ImGuiInputTextCallbackData;

namespace Editor {

    /**
     * @brief 콘솔 로그와 명령 입력을 표시하는 에디터 패널입니다.
     * @ingroup Editor
     */
    class FConsolePanel final : public Editor::FEditorPanel {
    private:
        class FOutput;

    public:
        /**
         * @brief FConsolePanel의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Context 패널이 조회하고 편집할 에디터 컨텍스트입니다.
         */
        explicit FConsolePanel(Editor::FEditorContext* Context);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FConsolePanel() override;

    public:
        /**
         * @brief 콘솔 패널에 메시지를 추가하고 스크롤 갱신을 요청합니다.
         * @param[in] Message 보고할 오류 내용입니다.
         */
        void AddLog(const Core::FString& Message);

    private:
        /**
         * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         */
        void DrawContent(float DeltaTime) override;

        /**
         * @brief 선택·스크롤 상태를 반영해 자동 완성 후보 목록을 그립니다.
         * @param[in] Height 픽셀 높이 또는 예약할 UI 높이입니다.
         */
        void DrawSuggestions(float Height);

        /**
         * @brief 자동 완성과 명령 기록 탐색을 지원하는 콘솔 입력란을 그립니다.
         */
        void DrawCommandInput();

        /**
         * @brief 현재 입력에 맞는 자동 완성 후보와 선택 위치를 갱신합니다.
         * @param[in] Input 해석할 원본 입력 문자열입니다.
         */
        void UpdateSuggestions(std::string_view Input);

        /**
         * @brief 위·아래 방향키에 따라 명령 기록과 편집 중이던 입력을 복원합니다.
         * @param[in] Data ImGui 입력 콜백 데이터입니다.
         */
        void BrowseHistory(ImGuiInputTextCallbackData& Data);

        /**
         * @brief ImGui 입력 콜백을 자동 완성과 명령 기록 처리에 연결합니다.
         * @param[in] Data ImGui 입력 콜백 데이터입니다.
         * @return ImGui 입력 콜백의 처리 결과입니다.
         */
        static int HandleInput(ImGuiInputTextCallbackData* Data);

    private:
        /**
         * @brief 콘솔 로그 출력·필터 구현 객체입니다.
         */
        std::unique_ptr<Editor::FConsolePanel::FOutput> mOutput{};

        /**
         * @brief 널 종료된 콘솔 명령 입력 버퍼입니다.
         */
        std::array<char, 512> mConsoleInput{};

        /**
         * @brief 실행한 콘솔 입력의 최근 기록입니다.
         */
        Core::TArray<Core::FString> mCommandHistory{};

        /**
         * @brief 기록 탐색 전에 편집하던 입력입니다.
         */
        Core::FString mHistoryDraft{};

        /**
         * @brief 현재 탐색하는 명령 기록 위치입니다.
         */
        std::size_t mHistoryPosition{};

        /**
         * @brief 현재 입력과 일치하는 자동 완성 목록입니다.
         */
        Core::TArray<Core::FString> mSuggestions{};

        /**
         * @brief 선택한 자동 완성 후보 인덱스입니다.
         */
        std::size_t mSelectedSuggestion{};

        /**
         * @brief 선택한 후보가 보이도록 스크롤할지 나타냅니다.
         */
        bool mScrollToSuggestion{};

        /**
         * @brief 입력 콜백의 자동 완성 적용 요청입니다.
         */
        bool mCompletionRequested{};
    };

}
