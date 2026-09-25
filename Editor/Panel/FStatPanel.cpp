#include "pch.h"
#include "FStatPanel.h"

FStatPanel::FStatPanel(UWorld& InWorld, FStateChannel<FStatDisplayFlags>::FReader InReader)
    : FEditorWindow("Stats"),
      mWorld(&InWorld),
      mModeReader(std::move(InReader)) {
    //bVisible = false;
}

bool FStatPanel::CheckVisible() {
    return !(mModeReader.Peek().mBShowFps || mModeReader.Peek().mBShowMemory || mModeReader.Peek().mBObjectSystem);
}

void FStatPanel::DrawContents() {
    DrawStatContents(*mWorld, mModeReader.Peek());
}
