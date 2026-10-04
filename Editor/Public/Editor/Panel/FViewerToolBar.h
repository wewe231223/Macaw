#pragma once
#include "Editor/Panel/IEditorPanel.h"
#include "Editor/World/FWorldEditorContext.h"

class FViewerToolBar final : public IEditorPanel {
public:
    FViewerToolBar(FWorldEditorContext& InEditorContext);
    ;

    ~FViewerToolBar() = default;

    FViewerToolBar(const FViewerToolBar&) = delete;
    FViewerToolBar& operator=(const FViewerToolBar&) = delete;

    FViewerToolBar(FViewerToolBar&&) = delete;
    FViewerToolBar& operator=(FViewerToolBar&&) = delete;

    void DrawPanel() override;

private:
    FWorldEditorContext* mEditorContext{nullptr};
};
