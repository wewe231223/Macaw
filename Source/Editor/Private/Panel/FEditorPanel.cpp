#include "Editor/Panel/FEditorPanel.h"

#include <imgui.h>
#include <utility>

FEditorPanel::FEditorPanel(UEditorEngine& Engine, FString Name, bool Closable)
	: mEngine{&Engine},
	  mName{std::move(Name)},
	  mClosable{Closable} {
}

FEditorPanel::~FEditorPanel() = default;

void FEditorPanel::Draw(float DeltaTime) {
    if (!mVisible) {
        return;
    }

    if (ImGui::Begin(mName.c_str(), mClosable ? &mVisible : nullptr)) {
        DrawContent(DeltaTime);
    }

    ImGui::End();
}

const FString& FEditorPanel::GetName() const {
    return mName;
}

bool FEditorPanel::IsVisible() const {
    return mVisible;
}

void FEditorPanel::SetVisible(bool Visible) {
    mVisible = Visible;
}

UEditorEngine& FEditorPanel::GetEditorEngine() const {
    return *mEngine;
}
