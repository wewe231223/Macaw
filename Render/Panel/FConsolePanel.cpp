#include "pch.h"
#include "FConsolePanel.h"

FConsolePanel::FConsolePanel(FConsoleOutputHandle InHandle, FStateChannel<FStatDisplayFlags>::FWriter Writer)
    : FEditorWindow("Console"),
      mHandle(InHandle),
      mModeWriter(std::move(Writer)) {
}

void FConsolePanel::DrawContents() {
    DrawConsoleContents(mHandle, mModeWriter);
}
