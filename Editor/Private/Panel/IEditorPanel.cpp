#include "pch.h"
#include "Editor/Panel/IEditorPanel.h"

void IEditorPanel::RenderOffscreen(FRenderer&) {
}

void IEditorPanel::ReleaseRenderResources() {
}

bool IEditorPanel::IsVisible() const {
    return mBVisible;
}

void IEditorPanel::SetVisible(bool BInVisible) {
    mBVisible = BInVisible;
}
