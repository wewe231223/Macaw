#include "Editor/Panel/FConsolePanel.h"
#include "Editor/EditorEngine.h"

#include <imgui.h>

FConsolePanel::FConsolePanel(UEditorEngine& Engine)
	: FEditorPanel{Engine, "Console", true} {
}

FConsolePanel::~FConsolePanel() = default;

void FConsolePanel::AddLog(const FString& Message) {
    if (mLogs.size() >= 500) {
        mLogs.erase(mLogs.begin(), mLogs.begin() + 100);
    }

    mLogs.push_back(Message);
    mScrollToBottom = true;
}

void FConsolePanel::Clear() {
    mLogs.clear();
    mScrollToBottom = false;
}

void FConsolePanel::DrawContent(float) {
    if (ImGui::BeginChild("Messages", {0.0f, -ImGui::GetFrameHeightWithSpacing()}, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
        for (const FString& Message : mLogs) {
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
        GetEditorEngine().ExecuteConsoleCommand(mConsoleInput.data());
        mConsoleInput.fill(0);
        ImGui::SetKeyboardFocusHere(-1);
    }
}
