#pragma once

#include "Core/ReflectionMacros.h"
#include "Editor/Panel/FEditorPanel.h"

#include <array>

class FConsolePanel final : public FEditorPanel {
public:
    ReflectTypes(FConsolePanel, FEditorPanel);

public:
    explicit FConsolePanel(UEditorEngine& Engine);
    ~FConsolePanel() override;

public:
    ReflectStruct(FConsolePanel);

    void AddLog(const FString& Message);
    void Clear();

private:
    void DrawContent(float DeltaTime) override;

private:
    ReflectMembers();

private:
    TArray<FString> mLogs{};
    ReflectProperty(mLogs, "Logs", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::array<char, 512> mConsoleInput{};
    ReflectProperty(mConsoleInput, "ConsoleInput", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mScrollToBottom{};
    ReflectProperty(mScrollToBottom, "ScrollToBottom", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
