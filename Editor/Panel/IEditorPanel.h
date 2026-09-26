#pragma once
#include "Render/Renderer.h"
#include "Asset/FAssetRegistry.h"

class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;

    virtual void DrawPanel() = 0;

    virtual void RenderOffscreen(FRenderer&, FAssetRegistry&);

    virtual void ReleaseRenderResources();

    bool IsVisible() const;

    void SetVisible(bool BInVisible);

protected:
    bool mBVisible{true};
};
