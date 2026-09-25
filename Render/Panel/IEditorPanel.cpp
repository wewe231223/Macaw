#include "pch.h"
#include "IEditorPanel.h"

void IEditorPanel::RenderOffscreen(FRenderer&, FAssetRegistry&) {
}

void IEditorPanel::ReleaseRenderResources() {
}

bool IEditorPanel::IsVisible() const {
    return mBVisible;
}

void IEditorPanel::SetVisible(bool BInVisible) {
    mBVisible = BInVisible;
}
