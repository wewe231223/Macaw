#include "pch.h"
#include "Editor/Panel/FConsolePanel.h"

FConsolePanel::FConsolePanel(FConsoleOutputHandle InHandle, FStatDisplayFlags& Flags)
	: FEditorWindow("Console"),
	  mHandle(InHandle),
	  mStatDisplayFlags(Flags) {
}

void FConsolePanel::DrawContents() {
    DrawConsoleContents(mHandle, mStatDisplayFlags);
}
