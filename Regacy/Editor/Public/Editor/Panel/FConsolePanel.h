#pragma once
#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Panel/Console/ConsoleWindow.h"
#include "Editor/Message/FEditorInfo.h"

class FConsolePanel : public FEditorWindow {
public:
    explicit FConsolePanel(FConsoleOutputHandle InHandle, FStatDisplayFlags& Flags);

private:
    void DrawContents() override;

private:
    FConsoleOutputHandle mHandle{};
    FStatDisplayFlags& mStatDisplayFlags;
};
