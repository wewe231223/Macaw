#pragma once

#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Panel/Console/ConsoleWindow.h"
#include "Core/Channel/FEditorInfo.h"

class FConsolePanel : public FEditorWindow {
public:
    explicit FConsolePanel(FConsoleOutputHandle InHandle, FStateChannel<FStatDisplayFlags>::FWriter Writer);

private:
    void DrawContents() override;

    FConsoleOutputHandle mHandle{};

    FStateChannel<FStatDisplayFlags>::FWriter mModeWriter{};
};
