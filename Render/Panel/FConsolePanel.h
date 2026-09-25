#pragma once

#include "Render/Panel/FEditorWindow.h"
#include "Render/Panel/Console/ConsoleWindow.h"
#include "FEditorInfo.h"

class FConsolePanel : public FEditorWindow {
public:
    explicit FConsolePanel(FConsoleOutputHandle InHandle, FStateChannel<FStatDisplayFlags>::FWriter Writer);

private:
    void DrawContents() override;

    FConsoleOutputHandle mHandle{};

    FStateChannel<FStatDisplayFlags>::FWriter mModeWriter{};
};
