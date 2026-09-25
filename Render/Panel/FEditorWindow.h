#pragma once

#include "IEditorPanel.h"
#include "ImGui/imgui.h"

class FEditorWindow : public IEditorPanel {
public:
    explicit FEditorWindow(const char* InWindowName, ImGuiWindowFlags InWindowFlags = ImGuiWindowFlags_None);

    ~FEditorWindow() override = default;

    FEditorWindow(const FEditorWindow&) = delete;
    FEditorWindow& operator=(const FEditorWindow&) = delete;
    FEditorWindow(FEditorWindow&&) = delete;
    FEditorWindow& operator=(FEditorWindow&&) = delete;

    void DrawPanel() final;

    const char* GetWindowName() const;

protected:
    virtual void DrawContents() = 0;

    virtual void PushWindowStyle();

    virtual void PopWindowStyle();

private:
    const char* mWindowName{nullptr};
    ImGuiWindowFlags mWindowFlags{ImGuiWindowFlags_None};
};
