#include "pch.h"
#include "Render/FSceneRenderer.h"
#include "Render/FFrameResource.h"
#include "Core/Stat/Stat.h"

bool FSceneRenderer::Initialize(ID3D11Device* Device) {
    Reset();

    if (Device == nullptr || !mTextRenderer.Initialize(Device, 256) || !mBillboardRenderer.Initialize(Device, 64) || !mPostProcessingRenderer.Initialize(Device)) {
        Reset();
        return false;
    }

    mDevice = Device;

    return true;
}

void FSceneRenderer::BeginFrame(Uint64 FrameSerial) {
    mFrameSerial = FrameSerial;

    constexpr Uint64 MaximumUnusedFrames{120};
    std::erase_if(mRenderQueues, [FrameSerial](const auto& Entry) {
        return FrameSerial - Entry.second.mLastUsedFrame > MaximumUnusedFrames;
    });
}

void FSceneRenderer::ResetScenes() {
    mRenderQueues.clear();
    mRenderScenes.clear();
}

void FSceneRenderer::Reset() {
    ResetScenes();
    mTextRenderer = {};
    mBillboardRenderer = {};
    mPostProcessingRenderer.Reset();
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

    const D3D11_VIEWPORT& Viewport{View.mTarget->GetViewport()};
    const Uint32 Width{static_cast<Uint32>(Viewport.Width)};
    const Uint32 Height{static_cast<Uint32>(Viewport.Height)};

    if (ViewQueue.mSceneColor == nullptr) {
        ViewQueue.mSceneColor = std::make_unique<FSceneRenderSurface>();
        ViewQueue.mSceneColor->InitializeOffscreen(mDevice, Width, Height);
    } else if (!ViewQueue.mSceneColor->Resize(mDevice, Width, Height)) {
        return {};
    }

    if (!ViewQueue.mSceneColor->IsValid()) {
        return {};
    }

    FRenderView SceneView{View};

    SceneView.mTarget = ViewQueue.mSceneColor.get();
    SceneView.SetPassEnabled(ERenderPass::Gizmo, false);

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::RenderQueue};

        ViewQueue.mQueue.Build(Context.mAssetRegistry, Scene, SceneView, Context.mAssetResources->GetMaterialBuffer());
    }

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::ViewBuffers};

        if (!Context.mFrameResource->PrepareView(mDevice, Context.mDeviceContext, SceneView, Scene, ViewQueue.mQueue)) {
            return {};
        }
    }

    ViewQueue.mSceneColor->Clear(Context.mDeviceContext, ClearColor);
    ViewQueue.mSceneColor->Bind(Context.mDeviceContext, View.mTarget->GetDepthStencilView());

    if (View.IsPassEnabled(ERenderPass::Opaque)) {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::Geometry};

        mMeshRenderer.Draw(Context, ViewQueue.mQueue.GetItems(ERenderPass::Opaque), View.mRenderMode, true);
    }

    if (View.IsPassEnabled(ERenderPass::Translucent)) {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::Geometry};

        mMeshRenderer.Draw(Context, ViewQueue.mQueue.GetItems(ERenderPass::Translucent), View.mRenderMode, true);
    }

    RenderTextAndBillboards(Context, SceneView, Scene);

    if (!mPostProcessingRenderer.Render(Context.mDeviceContext, ViewQueue.mSceneColor->GetShaderResourceView(), *View.mTarget, View.mSettings.mPostProcessing, View.IsPassEnabled(ERenderPass::PostProcessing))) {
        return {};
    }

    return FSceneRenderOutput{View.mTarget, View.mTarget->GetShaderResourceView(), View.mTarget->GetDepthShaderResourceView(), View.mTarget->GetDepthStencilView(), View.mTarget->GetViewport()};
}

void FSceneRenderer::RenderTextAndBillboards(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene) {
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
