#pragma once

#include "Render/FRenderContext.h"
#include "Render/FRenderQueue.h"
#include "Render/FRenderView.h"
#include "Render/FSceneRenderOutput.h"
#include "Render/FMeshRenderer.h"
#include "Render/FTextRenderer.h"
#include "Render/FBillboardRenderer.h"
#include "Render/FOffScreenRenderSurface.h"
#include "Render/FPostProcessingRenderer.h"

class FSceneRenderer {
private:
    struct FViewSurface {
        std::unique_ptr<FOffScreenRenderSurface> mSceneColor{};
        Uint64 mSceneId{};
        Uint64 mLastUsedFrame{};
    };

public:
    bool Initialize(ID3D11Device* Device);
    void BeginFrame(Uint64 FrameSerial);
    void ReleaseScene(Uint64 SceneId);
    void ResetScenes();
    void Reset();

    FSceneRenderOutput RenderView(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene);

private:
    void RenderTextAndBillboards(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene);

private:
    ID3D11Device* mDevice{nullptr};
    Uint64 mFrameSerial{};
    TMap<const FSceneRenderSurface*, FViewSurface> mViews{};
    TArray<FVisibleMeshDrawCommand> mVisibleCommands{};
    FRenderQueue mRenderQueue{};
    FMeshRenderer mMeshRenderer{};
    FTextRenderer mTextRenderer{};
    FBillboardRenderer mBillboardRenderer{};
    FPostProcessingRenderer mPostProcessingRenderer{};
};
