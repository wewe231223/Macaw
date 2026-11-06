/**
 * @file FConsolePanel.cpp
 * @brief 콘솔 로그와 명령 입력을 표시하는 에디터 패널입니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FConsolePanel.h"
#include "Editor/FEditorContext.h"

#include <imgui.h>
#include <imterm/terminal_helpers.hpp>

#include <algorithm>

namespace Editor {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief ImTerm 터미널이 사용할 에디터 로그 출력 기능을 연결합니다.
         */
        class FImTermOutputHelper final : public ImTerm::basic_terminal_helper<FImTermOutputHelper, void> {
        };

        /**
         * @brief 콘솔에 보관할 최대 명령 기록 수입니다.
         */
        constexpr std::size_t HistoryLimit{100};

        /**
         * @brief ImGui 입력 버퍼를 교체하고 커서를 끝으로 옮깁니다.
         * @param[in] Data ImGui 입력 콜백 데이터입니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 버퍼의 문자열을 교체했으면 true입니다.
         */
        bool ReplaceInput(ImGuiInputTextCallbackData& Data, const Core::FString& Text) {
            if (Text.size() >= static_cast<std::size_t>(Data.BufSize)) {
                return false;
            }

            Data.DeleteChars(0, Data.BufTextLen);
            Data.InsertChars(0, Text.data(), Text.data() + Text.size());

            return true;
        }

        /**
         * @brief 현재 테마의 콘솔 강조 색상을 반환합니다.
         * @return 현재 테마의 콘솔 강조 색상입니다.
         */
        ImVec4 GetConsoleAccent() {
            const ImVec4& Link{ImGui::GetStyleColorVec4(ImGuiCol_TextLink)};
            const ImVec4& Text{ImGui::GetStyleColorVec4(ImGuiCol_Text)};

            return {0.6f * Link.x + 0.4f * Text.x, 0.6f * Link.y + 0.4f * Text.y, 0.6f * Link.z + 0.4f * Text.z, Text.w};
        }
    }

    /**
     * @brief 편집 로그를 ImTerm 출력·필터에 동기화합니다.
     */
    class Editor::FConsolePanel::FOutput {
    public:
        /**
         * @brief 콘솔 로그를 그릴 ImTerm 터미널과 필터 상태를 준비합니다.
         */
        FOutput();

    public:
        /**
         * @brief 로그와 선택적 필터를 예약된 UI 높이에 맞춰 그립니다.
         * @param[in] Context 로그 원본인 편집 컨텍스트입니다.
         * @param[in] ReservedHeight 입력·후보 목록에 남겨 둘 높이입니다.
         * @param[in] ShowFilter 콘솔 필터 입력란의 표시 여부입니다.
         */
        void Draw(const Editor::FEditorContext& Context, float ReservedHeight, bool ShowFilter);

    private:
        /**
         * @brief 편집 컨텍스트의 로그 변경분을 콘솔 터미널에 반영합니다.
         * @param[in] Context 로그 원본인 편집 컨텍스트입니다.
         */
        void SyncLogs(const Editor::FEditorContext& Context);

    private:
        /**
         * @brief 로그를 필터링하고 그리는 ImTerm 터미널입니다.
         */
        ImTerm::terminal<FImTermOutputHelper> mTerminal{};

        /**
         * @brief 터미널 로그 원본 컨텍스트의 비소유 주소입니다.
         */
        const Editor::FEditorContext* mLogContext{};

        /**
         * @brief 마지막으로 반영한 로그 리비전입니다.
         */
        Core::Uint64 mLastLogRevision{};

        /**
         * @brief 터미널에 반영한 원본 로그 항목 수입니다.
         */
        std::size_t mLogCount{};
    };

    /**
     * @brief 콘솔 로그를 그릴 ImTerm 터미널과 필터 상태를 준비합니다.
     */
    Editor::FConsolePanel::FOutput::FOutput() {
        mTerminal.filter_hint() = "Filter output";
    }

    /**
     * @brief 편집 로그와 필터를 예약된 UI 높이에 맞춰 그립니다.
     */
    void Editor::FConsolePanel::FOutput::Draw(const Editor::FEditorContext& Context, float ReservedHeight, bool ShowFilter) {
        SyncLogs(Context);

        const ImVec4& Background{ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg)};
        const ImVec4 Accent{GetConsoleAccent()};
        ImTerm::theme& Theme{mTerminal.theme()};
        Theme.message_panel = ImTerm::theme::constexpr_color{Background.x, Background.y, Background.z, Background.w};
        Theme.log_level_colors[ImTerm::message::severity::debug] = ImTerm::theme::constexpr_color{Accent.x, Accent.y, Accent.z, Accent.w};
        Theme.matching_text = Theme.log_level_colors[ImTerm::message::severity::debug];

        mTerminal.DrawOutput(ReservedHeight, ShowFilter);
    }

    /**
     * @brief 편집 컨텍스트의 로그 변경분을 콘솔 터미널에 반영합니다.
     */
    void Editor::FConsolePanel::FOutput::SyncLogs(const Editor::FEditorContext& Context) {
        const Core::Uint64 Revision{Context.GetLogRevision()};

        if (mLogContext == &Context && Revision == mLastLogRevision) {
            return;
        }

        const Core::TArray<Core::FString>& Logs{Context.GetLogs()};

        if (mLogContext != &Context || Logs.size() < mLogCount || Revision - mLastLogRevision != Logs.size() - mLogCount) {
            mTerminal.clear();
            mLogCount = 0;
        }

        for (; mLogCount < Logs.size(); ++mLogCount) {
            const Core::FString& Message{Logs[mLogCount]};
            const bool IsCommand{Message.starts_with("> ")};
            const ImTerm::message::severity::severity_t Severity{IsCommand ? ImTerm::message::severity::debug : ImTerm::message::severity::info};

            mTerminal.add_message({Severity, Message, 0, IsCommand ? Message.size() : std::size_t{}, false});
        }

        mLogContext = &Context;
        mLastLogRevision = Revision;
    }

    /**
     * @brief FConsolePanel의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Editor::FConsolePanel::FConsolePanel(Editor::FEditorContext* Context)
		: Editor::FEditorPanel{Context, "Console", true},
		  mOutput{std::make_unique<Editor::FConsolePanel::FOutput>()} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Editor::FConsolePanel::~FConsolePanel() = default;

    /**
     * @brief 콘솔 패널에 메시지를 추가하고 스크롤 갱신을 요청합니다.
     */
    void Editor::FConsolePanel::AddLog(const Core::FString& Message) {
        GetEditorContext()->AddLog(Message);
    }

    /**
     * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
     */
    void Editor::FConsolePanel::DrawContent(float) {
        const float InputHeight{ImGui::GetFrameHeightWithSpacing()};
        const ImGuiStyle& Style{ImGui::GetStyle()};
        const float LineHeight{ImGui::GetTextLineHeightWithSpacing()};
        const ImVec2 Available{ImGui::GetContentRegionAvail()};
        const bool ShowFilter{Available.y >= 4.0f * InputHeight && Available.x >= 10.0f * ImGui::GetFontSize()};
        const float SuggestionRows{static_cast<float>(std::min(mSuggestions.size(), std::size_t{6}))};
        const float SuggestionLimit{Available.y - (ShowFilter ? 3.0f : 2.0f) * InputHeight - Style.ItemSpacing.y};
        const float SuggestionHeight{std::min(SuggestionRows * LineHeight + 2.0f * Style.WindowPadding.y, SuggestionLimit)};
        const bool ShowSuggestions{!mSuggestions.empty() && SuggestionHeight >= LineHeight + 2.0f * Style.WindowPadding.y};
        const float ReservedHeight{InputHeight + (ShowSuggestions ? SuggestionHeight + Style.ItemSpacing.y : 0.0f)};

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, std::max(Style.ChildRounding, Style.FrameRounding));
        mOutput->Draw(*GetEditorContext(), ReservedHeight, ShowFilter);

        if (ShowSuggestions) {
            DrawSuggestions(SuggestionHeight);
        }

        ImGui::PopStyleVar();
        DrawCommandInput();
    }

    /**
     * @brief 선택·스크롤 상태를 반영해 자동 완성 후보 목록을 그립니다.
     */
    void Editor::FConsolePanel::DrawSuggestions(float Height) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_PopupBg));

        if (ImGui::BeginChild("CommandSuggestions", {0.0f, Height}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoNav)) {
            for (std::size_t Index{}; Index < mSuggestions.size(); ++Index) {
                const bool Selected{Index == mSelectedSuggestion};

                ImGui::PushID(static_cast<int>(Index));
                ImGui::Selectable(mSuggestions[Index].c_str(), Selected);

                if (ImGui::IsItemClicked()) {
                    mSelectedSuggestion = Index;
                    mCompletionRequested = true;
                }

                if (Selected && mScrollToSuggestion) {
                    ImGui::SetScrollHereY();
                }

                ImGui::PopID();
            }

            mScrollToSuggestion = false;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    /**
     * @brief 자동 완성과 명령 기록 탐색을 지원하는 콘솔 입력란을 그립니다.
     */
    void Editor::FConsolePanel::DrawCommandInput() {
        const ImGuiStyle& Style{ImGui::GetStyle()};

        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(GetConsoleAccent(), ">");
        ImGui::SameLine(0.0f, Style.ItemInnerSpacing.x);

        const float InputX{ImGui::GetCursorPosX()};
        const float Width{ImGui::GetContentRegionAvail().x};
        const float RunWidth{ImGui::CalcTextSize("Run").x + 2.0f * Style.FramePadding.x};
        const bool ShowRun{Width >= RunWidth + Style.ItemSpacing.x + 6.0f * ImGui::GetFontSize()};
        bool RunClicked{};

        if (ShowRun) {
            ImGui::SetCursorPosX(InputX + Width - RunWidth);
            ImGui::BeginDisabled(mConsoleInput.front() == '\0');

            RunClicked = ImGui::Button("Run", {RunWidth, 0.0f});

            ImGui::EndDisabled();
            ImGui::SetItemTooltip("Run (Enter)\nCommand history (Up / Down)\nSuggestions (Ctrl+Up / Ctrl+Down, Tab)");
            ImGui::SameLine(InputX, 0.0f);
        }

        if (mCompletionRequested || RunClicked) {
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::SetNextItemWidth(ShowRun ? Width - RunWidth - Style.ItemSpacing.x : -1.0f);

        constexpr ImGuiInputTextFlags Flags{ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackAlways};
        const bool EnterPressed{ImGui::InputTextWithHint("##Command", "Type a command or help", mConsoleInput.data(), mConsoleInput.size(), Flags, HandleInput, this)};
        const bool InputActive{ImGui::IsItemActive()};
        const bool InputActivated{ImGui::IsItemActivated()};

        if (EnterPressed) {
            ImGui::SetKeyboardFocusHere(-1);
        }

        if (EnterPressed || RunClicked) {
            const Core::FString Command{mConsoleInput.data()};

            GetEditorContext()->ExecuteConsoleCommand(Command);

            if (Command.find_first_not_of(" \t\r\n\f\v") != Core::FString::npos && (mCommandHistory.empty() || mCommandHistory.back() != Command)) {
                if (mCommandHistory.size() == HistoryLimit) {
                    mCommandHistory.erase(mCommandHistory.begin());
                }

                mCommandHistory.push_back(Command);
            }

            mHistoryPosition = mCommandHistory.size();
            mHistoryDraft.clear();
            mConsoleInput.fill(0);
            mSuggestions.clear();
        } else if (InputActivated && !mCompletionRequested && mHistoryPosition == mCommandHistory.size()) {
            UpdateSuggestions(mConsoleInput.data());
        } else if (!InputActive && !mCompletionRequested) {
            mSuggestions.clear();
        }
    }

    /**
     * @brief 현재 입력에 맞는 자동 완성 후보와 선택 위치를 갱신합니다.
     */
    void Editor::FConsolePanel::UpdateSuggestions(std::string_view Input) {
        mSuggestions = GetEditorContext()->GetConsoleSuggestions(Input);
        mSelectedSuggestion = 0;
        mScrollToSuggestion = true;
    }

    /**
     * @brief 위·아래 방향키에 따라 명령 기록과 편집 중이던 입력을 복원합니다.
     */
    void Editor::FConsolePanel::BrowseHistory(ImGuiInputTextCallbackData& Data) {
        if (Data.EventKey == ImGuiKey_UpArrow) {
            if (mHistoryPosition == 0) {
                return;
            }

            if (mHistoryPosition == mCommandHistory.size()) {
                mHistoryDraft.assign(Data.Buf, static_cast<std::size_t>(Data.BufTextLen));
            }

            --mHistoryPosition;
        } else {
            if (mHistoryPosition == mCommandHistory.size()) {
                return;
            }

            ++mHistoryPosition;
        }

        if (mHistoryPosition == mCommandHistory.size()) {
            ReplaceInput(Data, mHistoryDraft);
            UpdateSuggestions({Data.Buf, static_cast<std::size_t>(Data.BufTextLen)});
        } else {
            ReplaceInput(Data, mCommandHistory[mHistoryPosition]);
            mSuggestions.clear();
        }
    }

    /**
     * @brief ImGui 입력 콜백을 자동 완성과 명령 기록 처리에 연결합니다.
     */
    int Editor::FConsolePanel::HandleInput(ImGuiInputTextCallbackData* Data) {
        Editor::FConsolePanel& Panel{*static_cast<Editor::FConsolePanel*>(Data->UserData)};
        const std::size_t Count{Panel.mSuggestions.size()};

        if (Panel.mCompletionRequested || Data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
            Panel.mCompletionRequested = false;

            if (Count != 0) {
                const Core::FString& Suggestion{Panel.mSuggestions[Panel.mSelectedSuggestion]};

                if (ReplaceInput(*Data, Suggestion)) {
                    Panel.mHistoryPosition = Panel.mCommandHistory.size();
                    Panel.UpdateSuggestions({Data->Buf, static_cast<std::size_t>(Data->BufTextLen)});
                }
            }
        } else if (Data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
            if (!ImGui::GetIO().KeyCtrl) {
                Panel.BrowseHistory(*Data);
            } else if (Count != 0) {
                Panel.mSelectedSuggestion = Data->EventKey == ImGuiKey_UpArrow ? (Panel.mSelectedSuggestion + Count - 1) % Count : (Panel.mSelectedSuggestion + 1) % Count;
                Panel.mScrollToSuggestion = true;
            }
        } else if (Data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            Panel.mHistoryPosition = Panel.mCommandHistory.size();
            Panel.UpdateSuggestions({Data->Buf, static_cast<std::size_t>(Data->BufTextLen)});
        }

        return 0;
    }

}
