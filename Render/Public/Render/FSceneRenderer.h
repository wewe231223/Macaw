#pragma once

#include "Render/FRenderContext.h"
#include "Render/FRenderQueue.h"
#include "Render/FSceneRenderOutput.h"
#include "Render/FMeshRenderer.h"
#include "Render/FTextRenderer.h"
#include "Render/FBillboardRenderer.h"
#include "Render/FSceneRenderSurface.h"
#include "Render/FPostProcessingRenderer.h"

class FSceneRenderer {
private:
    struct FViewRenderQueue {
        FRenderQueue mQueue{};
        std::unique_ptr<FSceneRenderSurface> mSceneColor{};
        Uint64 mLastUsedFrame{};
    };

public:
    bool Initialize(ID3D11Device* Device);
    void BeginFrame(Uint64 FrameSerial);
    void ResetScenes();
    void Reset();

    const FRenderScene& SynchronizeScene(const FRenderContext& Context, FSceneRenderData& Scene);
    FSceneRenderOutput RenderView(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene);
    const FRenderQueue* GetRenderQueue(const IRenderSurface* Target) const;

private:
    void RenderTextAndBillboards(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene);

private:
    ID3D11Device* mDevice{nullptr};
    Uint64 mTransientSceneId{AllocateRenderSceneId()};
    Uint64 mFrameSerial{};
    TMap<Uint64, std::unique_ptr<FRenderScene>> mRenderScenes{};
    TMap<const IRenderSurface*, FViewRenderQueue> mRenderQueues{};
    FMeshRenderer mMeshRenderer{};
    FTextRenderer mTextRenderer{};
    FBillboardRenderer mBillboardRenderer{};
    FPostProcessingRenderer mPostProcessingRenderer{};
};
