#include "pch.h"
#include "Render/FSceneRenderer.h"
#include "Render/FFrameResource.h"
#include "Core/Stat/Stat.h"

bool FSceneRenderer::Initialize(ID3D11Device* Device) {
    Reset();

    if (Device == nullptr || !mTextRenderer.Initialize(Device, 256) || !mBillboardRenderer.Initialize(Device, 64) || !mOcclusionCulling.Initialize(Device)) {
        Reset();
        return false;
    }

    mDevice = Device;

    return true;
}

void FSceneRenderer::BeginFrame(Uint64 FrameSerial) {
    mFrameSerial = FrameSerial;
    mOcclusionCulling.BeginFrame(FrameSerial);

    constexpr Uint64 MaximumUnusedFrames{120};
    std::erase_if(mRenderQueues, [FrameSerial](const auto& Entry) {
        return FrameSerial - Entry.second.mLastUsedFrame > MaximumUnusedFrames;
    });
}

void FSceneRenderer::ResetScenes() {
    mOcclusionCulling.ResetViews();
    mRenderQueues.clear();
    mRenderScenes.clear();
}

void FSceneRenderer::Reset() {
    ResetScenes();
    mOcclusionCulling.Reset();
    mTextRenderer = {};
    mBillboardRenderer = {};
    mDevice = nullptr;
    mFrameSerial = 0;
}

const FRenderScene& FSceneRenderer::SynchronizeScene(const FRenderContext& Context, FSceneRenderData& Scene) {
    const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::SceneSynchronization};
    const Uint64 SceneId{Scene.mSceneId != 0 ? Scene.mSceneId : mTransientSceneId};
    std::unique_ptr<FRenderScene>& RenderScene{mRenderScenes[SceneId]};

    if (RenderScene == nullptr) {
        RenderScene = std::make_unique<FRenderScene>(SceneId);
    }

    if (Context.mAssetRegistry != nullptr) {
        Context.mAssetResources->GetMaterialBuffer().Synchronize(*Context.mAssetRegistry, Context.mDeviceContext);
    }

    RenderScene->Synchronize(Context.mAssetRegistry, Scene, Context.mAssetResources->GetMaterialBuffer());

    return *RenderScene;
}

FSceneRenderOutput FSceneRenderer::RenderView(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene) {
    if (mDevice == nullptr || Context.mDeviceContext == nullptr || Context.mFrameResource == nullptr || View.mTarget == nullptr || !View.mTarget->IsValid()) {
        return {};
    }

    ID3D11ShaderResourceView* NullResource{nullptr};

    Context.mDeviceContext->PSSetShaderResources(0, 1, &NullResource);
    View.mTarget->Bind(Context.mDeviceContext);

    const float ClearColor[]{View.mSettings.mClearColor.mX, View.mSettings.mClearColor.mY, View.mSettings.mClearColor.mZ, View.mSettings.mClearColor.mW};

    View.mTarget->Clear(Context.mDeviceContext, ClearColor);

    if (Context.mAssetRegistry == nullptr || Context.mAssetResources == nullptr) {
        return {};
    }

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::MaterialBuffer};

        Context.mAssetResources->GetMaterialBuffer().Synchronize(*Context.mAssetRegistry, Context.mDeviceContext);
    }

    FViewRenderQueue& ViewQueue{mRenderQueues[View.mTarget]};

    ViewQueue.mLastUsedFrame = mFrameSerial;

    FRenderView SceneView{View};

    SceneView.SetPassEnabled(ERenderPass::Gizmo, false);

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::RenderQueue};

        ViewQueue.mQueue.Build(Context.mAssetRegistry, Scene, SceneView, Context.mAssetResources->GetMaterialBuffer());
    }

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::ViewBuffers};

        if (!Context.mFrameResource->PrepareView(mDevice, Context.mDeviceContext, View, Scene, ViewQueue.mQueue)) {
            return {};
        }
    }

    if (View.IsPassEnabled(ERenderPass::SceneGeometry)) {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::Geometry};

        if (mOcclusionCulling.Prepare(mDevice, Context.mDeviceContext, View, Scene, ViewQueue.mQueue)) {
            mMeshRenderer.DrawOccluded(Context, View, ViewQueue.mQueue, mOcclusionCulling);
        } else {
            mMeshRenderer.Draw(Context, ViewQueue.mQueue.GetItems(ERenderPass::SceneGeometry), View.mRenderMode);
        }
    }

    return FSceneRenderOutput{View.mTarget, View.mTarget->GetShaderResourceView(), View.mTarget->GetDepthShaderResourceView(), View.mTarget->GetDepthStencilView(), View.mTarget->GetViewport()};
}

void FSceneRenderer::RenderTextAndBillboards(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene, ID3D11DepthStencilView* DepthStencilView) {
    View.mTarget->Bind(Context.mDeviceContext, DepthStencilView);

    if (View.IsPassEnabled(ERenderPass::Text)) {
        mTextRenderer.Render(Context.mDeviceContext, *Context.mFrameResource, Scene.GetTextProbes(), Context.mAssetRegistry, *Context.mAssetResources);
    }

    if (View.IsPassEnabled(ERenderPass::Billboard)) {
        mBillboardRenderer.Render(Context.mDeviceContext, *Context.mFrameResource, Scene.GetBillboardProbes(), Context.mAssetRegistry, *Context.mAssetResources, View.mRenderMode);
    }
}

const FRenderQueue* FSceneRenderer::GetRenderQueue(const IRenderSurface* Target) const {
    const auto Position{mRenderQueues.find(Target)};

    return Position != mRenderQueues.end() ? &Position->second.mQueue : nullptr;
}
