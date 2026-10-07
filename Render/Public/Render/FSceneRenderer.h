#pragma once

#include "Render/FRenderContext.h"
#include "Render/FRenderQueue.h"
#include "Render/FSceneRenderOutput.h"
#include "Render/FMeshRenderer.h"
#include "Render/FTextRenderer.h"
#include "Render/FBillboardRenderer.h"

class FSceneRenderer {
private:
    struct FViewRenderQueue {
        FRenderQueue mQueue{};
        Uint64 mLastUsedFrame{};
    };

public:
    bool Initialize(ID3D11Device* Device);
    void BeginFrame(Uint64 FrameSerial);
    void ResetScenes();
    void Reset();

    const FRenderScene& SynchronizeScene(const FRenderContext& Context, FSceneRenderData& Scene);
    FSceneRenderOutput RenderView(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene);
    void RenderTextAndBillboards(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene, ID3D11DepthStencilView* DepthStencilView);
    const FRenderQueue* GetRenderQueue(const IRenderSurface* Target) const;

private:
    ID3D11Device* mDevice{nullptr};
    Uint64 mTransientSceneId{AllocateRenderSceneId()};
    Uint64 mFrameSerial{};
    TMap<Uint64, std::unique_ptr<FRenderScene>> mRenderScenes{};
    TMap<const IRenderSurface*, FViewRenderQueue> mRenderQueues{};
    FMeshRenderer mMeshRenderer{};
    FTextRenderer mTextRenderer{};
    FBillboardRenderer mBillboardRenderer{};
};
