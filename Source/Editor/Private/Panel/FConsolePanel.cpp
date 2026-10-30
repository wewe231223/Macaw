/**
 * @file FConsolePanel.cpp
 * @brief 콘솔 로그와 명령 입력을 표시하는 에디터 패널입니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FConsolePanel.h"
#include "Editor/FEditorContext.h"

#include <imgui.h>

namespace Editor {

    /**
     * @brief FConsolePanel의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Editor::FConsolePanel::FConsolePanel(Editor::FEditorContext* Context)
		: Editor::FEditorPanel{Context, "Console", true} {
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
     * @brief 수집한 콘솔 로그를 모두 지웁니다.
     */
    void Editor::FConsolePanel::Clear() {
        GetEditorContext()->ClearLogs();
        mScrollToBottom = false;
    }

    /**
     * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
     */
    void Editor::FConsolePanel::DrawContent(float) {
        Editor::FEditorContext* Context{GetEditorContext()};

        if (mLastLogRevision != Context->GetLogRevision()) {
            mLastLogRevision = Context->GetLogRevision();
            mScrollToBottom = !Context->GetLogs().empty();
        }

        if (ImGui::BeginChild("Messages", {0.0f, -ImGui::GetFrameHeightWithSpacing()}, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
            for (const Core::FString& Message : Context->GetLogs()) {
                ImGui::TextUnformatted(Message.c_str());
            }

            if (mScrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
                mScrollToBottom = false;
            }
        }

        ImGui::EndChild();
        ImGui::SetNextItemWidth(-1.0f);

        if (ImGui::InputText("##Command", mConsoleInput.data(), mConsoleInput.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            GetEditorContext()->ExecuteConsoleCommand(mConsoleInput.data());
            mConsoleInput.fill(0);
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

}
