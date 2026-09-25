#include "pch.h"
#include "FViewerToolBar.h"
#include "ImGui/imgui.h"

void FViewerToolBar::DrawPanel() {
    if (ImGui::BeginMenu("Import")) {
        if (ImGui::MenuItem("Open Obj...")) {
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();
    int RenderIndex{static_cast<int>(mEditorContext->GetRenderModeState())};
    const char* RenderModes[]{"Lit", "Unlit", "Wireframe", "Lit Wireframe"};
    ImGui::SetNextItemWidth(110.0f);
    if (ImGui::Combo("Render Mode", &RenderIndex, RenderModes, IM_ARRAYSIZE(RenderModes))) {
        mEditorContext->SetRenderModeState(static_cast<std::size_t>(RenderIndex));
    }
}

FViewerToolBar::FViewerToolBar(FWorldEditorContext& InEditorContext)
    : mEditorContext(&InEditorContext) {
}
