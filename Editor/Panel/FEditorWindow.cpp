#include "pch.h"
#include "FEditorWindow.h"

FEditorWindow::FEditorWindow(const char* InWindowName, ImGuiWindowFlags InWindowFlags)
    : mWindowName(InWindowName),
      mWindowFlags(InWindowFlags) {
}

void FEditorWindow::DrawPanel() {
    PushWindowStyle();

    const bool BDrawContents{ImGui::Begin(mWindowName, &mBVisible, mWindowFlags)};
    if (BDrawContents) {
        DrawContents();
    }

    ImGui::End();
    PopWindowStyle();
}

const char* FEditorWindow::GetWindowName() const {
    return mWindowName;
}

void FEditorWindow::PushWindowStyle() {
}

void FEditorWindow::PopWindowStyle() {
}
