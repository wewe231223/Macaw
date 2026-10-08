#include "pch.h"
#include "Render/FSceneRenderer.h"
#include "Render/FFrameResource.h"
#include "Render/FRenderScene.h"
#include "Render/FViewElementCollector.h"
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
    std::erase_if(mViews, [FrameSerial](const auto& Entry) {
        return FrameSerial - Entry.second.mLastUsedFrame > MaximumUnusedFrames;
    });
}

void FSceneRenderer::ResetScenes() {
    mViews.clear();
    mVisibleCommands.clear();
    mRenderQueue = {};
}

void FSceneRenderer::ReleaseScene(Uint64 SceneId) {
    std::erase_if(mViews, [SceneId](const auto& Entry) {
        return Entry.second.mSceneId == SceneId;
    });
    mVisibleCommands.clear();
    mRenderQueue = {};
}

void FSceneRenderer::Reset() {
    ResetScenes();
    mTextRenderer = {};
    mBillboardRenderer = {};
    mPostProcessingRenderer.Reset();
    mDevice = nullptr;
    mFrameSerial = 0;
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

    FViewSurface& ViewState{mViews[View.mTarget]};

    ViewState.mLastUsedFrame = mFrameSerial;
    ViewState.mSceneId = Scene.GetId();

    const D3D11_VIEWPORT& Viewport{View.mTarget->GetViewport()};
    const Uint32 Width{static_cast<Uint32>(Viewport.Width)};
    const Uint32 Height{static_cast<Uint32>(Viewport.Height)};

    if (ViewState.mSceneColor == nullptr) {
        ViewState.mSceneColor = std::make_unique<FSceneRenderSurface>();
        ViewState.mSceneColor->InitializeOffscreen(mDevice, Width, Height);
    } else if (!ViewState.mSceneColor->Resize(mDevice, Width, Height)) {
        return {};
    }

    if (!ViewState.mSceneColor->IsValid()) {
        return {};
    }

    FRenderView SceneView{View};

    SceneView.mTarget = ViewState.mSceneColor.get();

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::RenderQueue};

        SceneView.CollectMeshDrawCommands(Scene, mVisibleCommands);
        mRenderQueue.Build(Scene.GetCachedMeshDrawCommands(), mVisibleCommands, SceneView.IsPassEnabled(ERenderPass::Opaque), SceneView.IsPassEnabled(ERenderPass::Translucent));
    }

    {
        const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::ViewBuffers};

        if (!Context.mFrameResource->PrepareView(mDevice, Context.mDeviceContext, SceneView, Scene, mRenderQueue)) {
            return {};
        }
    }

    ViewState.mSceneColor->Clear(Context.mDeviceContext, ClearColor);
    ViewState.mSceneColor->Bind(Context.mDeviceContext, View.mTarget->GetDepthStencilView());

    if (View.IsPassEnabled(ERenderPass::Opaque)) {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::Geometry};

        mMeshRenderer.Draw(Context, mRenderQueue.GetCommands(ERenderPass::Opaque), View.mRenderMode);
    }

    if (View.IsPassEnabled(ERenderPass::Translucent)) {
        const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::Geometry};

        mMeshRenderer.Draw(Context, mRenderQueue.GetCommands(ERenderPass::Translucent), View.mRenderMode);
    }

    RenderTextAndBillboards(Context, SceneView, Scene);

    if (!mPostProcessingRenderer.Render(Context.mDeviceContext, ViewState.mSceneColor->GetShaderResourceView(), *View.mTarget, View.mSettings.mPostProcessing, View.IsPassEnabled(ERenderPass::PostProcessing))) {
        return {};
    }

    return FSceneRenderOutput{View.mTarget, View.mTarget->GetShaderResourceView(), View.mTarget->GetDepthShaderResourceView(), View.mTarget->GetDepthStencilView(), View.mTarget->GetViewport()};
}

void FSceneRenderer::RenderTextAndBillboards(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene) {
    FViewElementCollector Collector{View};

    Scene.CollectDynamicMeshElements(Collector);

    if (View.IsPassEnabled(ERenderPass::Text)) {
        mTextRenderer.Render(Context.mDeviceContext, *Context.mFrameResource, Collector.GetTextDraws(), Context.mAssetRegistry, *Context.mAssetResources);
    }

    if (View.IsPassEnabled(ERenderPass::Billboard)) {
        mBillboardRenderer.Render(Context.mDeviceContext, *Context.mFrameResource, Collector.GetBillboardDraws(), Context.mAssetRegistry, *Context.mAssetResources, View.mRenderMode);
    }
}
