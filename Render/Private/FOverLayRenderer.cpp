#include "pch.h"
#include "Render/FOverLayRenderer.h"
#include "Render/RenderConfig.h"
#include "Asset/UMesh.h"
#include "Asset/UTexture.h"
#include "Core/Stat/Stat.h"

bool FOverLayRenderer::Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, Uint32 FrameResourceCount) {
    Reset();

    if (Device == nullptr || Context == nullptr || FrameResourceCount == 0) {
        return false;
    }

    mFrameResources.resize(FrameResourceCount);

    for (FFrameResource& FrameResource : mFrameResources) {
        if (!FrameResource.Initialize(Device, Context)) {
            Reset();
            return false;
        }
    }

    mLineRenderer.Initialize(Device);
    mDevice = Device;

    return true;
}

bool FOverLayRenderer::BindAssetRegistry(const IAssetRegistry* Registry) {
    if (mAssetRegistry == Registry) {
        return true;
    }

    mOutlineDraws.clear();
    mGizmoDraws.clear();
    mQueue = {};
    mViewDepths.clear();
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;
    mAssetResources.Reset();
    mAssetRegistry = nullptr;

    for (FFrameResource& FrameResource : mFrameResources) {
        FrameResource.ResetScenes();
    }

    if (Registry != nullptr && !mAssetResources.Initialize(mDevice)) {
        return false;
    }

    mAssetRegistry = Registry;

    return true;
}

bool FOverLayRenderer::BeginFrame(ID3D11DeviceContext* Context, Uint32 FrameResourceIndex, Uint64 FrameSerial, float AnimationTime) {
    if (mCurrentFrameResource != nullptr || FrameResourceIndex >= mFrameResources.size()) {
        return false;
    }

    FFrameResource& FrameResource{mFrameResources[FrameResourceIndex]};

    if (!FrameResource.BeginFrame(Context, AnimationTime)) {
        return false;
    }

    mCurrentFrameResource = &FrameResource;
    mFrameSerial = FrameSerial;
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;
    mOutlineDraws.clear();
    mGizmoDraws.clear();

    constexpr Uint64 MaximumUnusedFrames{120};
    std::erase_if(mViewDepths, [FrameSerial](const auto& Entry) {
        return FrameSerial - Entry.second.mLastUsedFrame > MaximumUnusedFrames;
    });

    if (mAssetRegistry != nullptr) {
        mAssetResources.Prune(*mAssetRegistry);
    }

    return true;
}

void FOverLayRenderer::EndFrame() {
    if (mCurrentFrameResource != nullptr) {
        mCurrentFrameResource->EndFrame();
    }

    mCurrentFrameResource = nullptr;
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;
}

void FOverLayRenderer::Reset() {
    EndFrame();
    mOutlineDraws.clear();
    mGizmoDraws.clear();
    mQueue = {};
    mViewDepths.clear();
    mLineRenderer.Reset();
    mFrameResources.clear();
    mAssetResources.Reset();
    mAssetRegistry = nullptr;
    mDevice = nullptr;
    mFrameSerial = 0;
}

ID3D11DepthStencilView* FOverLayRenderer::RenderView(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene, const FRenderQueue& SceneQueue, const FSceneRenderOutput& Output) {
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;

    if (Context.mDeviceContext == nullptr || mDevice == nullptr || mAssetRegistry == nullptr || mCurrentFrameResource == nullptr || !Output.IsValid()) {
        return Output.mDepthStencilView;
    }

    if (!View.IsPassEnabled(ERenderPass::SelectionOutline) && !View.IsPassEnabled(ERenderPass::SceneGuides) && !View.IsPassEnabled(ERenderPass::Gizmo) && !View.IsPassEnabled(ERenderPass::OrientationAxis)) {
        return Output.mDepthStencilView;
    }

    if (!mAssetResources.GetMaterialBuffer().Synchronize(*mAssetRegistry, Context.mDeviceContext)) {
        return Output.mDepthStencilView;
    }

    mQueue.BuildOverLay(mAssetRegistry, SceneQueue, View, Context.mAssetResources->GetMaterialBuffer(), mAssetResources.GetMaterialBuffer());

    if (!PrepareMeshDraws(mQueue.GetItems(ERenderPass::SelectionOutline), ERenderMode::Outline, mOutlineDraws) || !PrepareMeshDraws(mQueue.GetItems(ERenderPass::Gizmo), ERenderMode::Lit, mGizmoDraws) || !mCurrentFrameResource->PrepareView(mDevice, Context.mDeviceContext, View, Scene, mQueue) || !PrepareDepth(Context.mDeviceContext, Output)) {
        return Output.mDepthStencilView;
    }

    Output.mTarget->Bind(Context.mDeviceContext, mCurrentDepth);
    DrawMeshes(Context.mDeviceContext, mOutlineDraws);

    if (View.IsPassEnabled(ERenderPass::SceneGuides)) {
        DrawSceneGuides(Context.mDeviceContext, View);
    }

    if (View.IsPassEnabled(ERenderPass::Gizmo) && !View.mGizmoProbes.empty()) {
        Context.mDeviceContext->ClearDepthStencilView(mCurrentDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
        DrawMeshes(Context.mDeviceContext, mGizmoDraws);
    }

    return mCurrentDepth;
}

bool FOverLayRenderer::PrepareDepth(ID3D11DeviceContext* Context, const FSceneRenderOutput& Output) {
    Microsoft::WRL::ComPtr<ID3D11Resource> Resource{};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Source{};

    Output.mDepthResource->GetResource(Resource.GetAddressOf());

    if (FAILED(Resource.As(&Source))) {
        return false;
    }

    D3D11_TEXTURE2D_DESC Description{};
    D3D11_TEXTURE2D_DESC CurrentDescription{};

    Source->GetDesc(&Description);

    FViewDepth& Depth{mViewDepths[Output.mTarget]};

    if (Depth.mTexture != nullptr) {
        Depth.mTexture->GetDesc(&CurrentDescription);
    }

    if (Depth.mTexture == nullptr || Description.Width != CurrentDescription.Width || Description.Height != CurrentDescription.Height || Description.Format != CurrentDescription.Format || Description.SampleDesc.Count != CurrentDescription.SampleDesc.Count || Description.SampleDesc.Quality != CurrentDescription.SampleDesc.Quality) {
        FViewDepth Replacement{};
        D3D11_DEPTH_STENCIL_VIEW_DESC ViewDescription{};

        Output.mDepthStencilView->GetDesc(&ViewDescription);
        Description.Usage = D3D11_USAGE_DEFAULT;
        Description.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        Description.CPUAccessFlags = 0;
        Description.MiscFlags = 0;
        ViewDescription.Flags = 0;

        if (FAILED(mDevice->CreateTexture2D(&Description, nullptr, Replacement.mTexture.GetAddressOf())) || FAILED(mDevice->CreateDepthStencilView(Replacement.mTexture.Get(), &ViewDescription, Replacement.mView.GetAddressOf()))) {
            return false;
        }

        Depth = std::move(Replacement);
    }

    Context->OMSetRenderTargets(0, nullptr, nullptr);
    Context->CopyResource(Depth.mTexture.Get(), Source.Get());
    Depth.mLastUsedFrame = mFrameSerial;
    mCurrentDepth = Depth.mView.Get();
    mCurrentTarget = Output.mTarget;

    return true;
}

bool FOverLayRenderer::PrepareMeshDraws(const TArray<FMeshDrawBatch>& Items, ERenderMode Mode, TArray<FMeshDraw>& Draws) {
    Draws.clear();
    Draws.reserve(Items.size());

    for (const FMeshDrawBatch& Item : Items) {
        const UPipeline* Pipeline{mAssetRegistry->ResolveAsset<UPipeline>(Item.mState.mPipelineHandle)};
        const UMesh* Mesh{mAssetRegistry->ResolveAsset<UMesh>(Item.mState.mMeshHandle)};

        if (Pipeline == nullptr || Mesh == nullptr || Item.mRecordCount == 0 || (Mode == ERenderMode::Outline && !Pipeline->RenderModeSettable(Mode))) {
            continue;
        }

        FMeshDraw Draw{};

        Draw.mMode = Pipeline->ResolveRenderMode(Mode);

        if (!Pipeline->RenderModeSettable(Draw.mMode)) {
            continue;
        }

        Draw.mMesh = mAssetResources.GetMesh(*Mesh);
        Draw.mPipeline = mAssetResources.GetPipeline(*Pipeline);

        if (Draw.mMesh == nullptr || Draw.mPipeline == nullptr) {
            return false;
        }

        Draw.mBatch = Item;
        Draw.mStencilReference = Draw.mMode == ERenderMode::Outline || (Item.mFlags & static_cast<Uint32>(ERenderObjectFlags::Selected)) != 0 ? 1u : 0u;
        Draw.mVertexStrides = {Mesh->GetVertexStride(EVertexAttribute::Position), Mesh->GetVertexStride(EVertexAttribute::Normal), Mesh->GetVertexStride(EVertexAttribute::UV), Mesh->GetVertexStride(EVertexAttribute::Color)};

        for (Uint8 Index{}; Index < Item.mState.mTextureSignature.mTextureFieldCount; ++Index) {
            const UTexture* Texture{mAssetRegistry->ResolveAsset<UTexture>(Item.mState.mTextureSignature.GetTextureHandle(Index))};

            Draw.mTextures[Index] = Texture != nullptr ? mAssetResources.GetTexture(*Texture) : nullptr;
        }

        Draws.push_back(Draw);
    }

    return true;
}

void FOverLayRenderer::DrawMeshes(ID3D11DeviceContext* Context, const TArray<FMeshDraw>& Draws) {
    if (Draws.empty() || !mCurrentFrameResource->BindModels(Context)) {
        return;
    }

    ID3D11ShaderResourceView* Materials{mAssetResources.GetMaterialBuffer().GetSRV()};

    Context->VSSetShaderResources(1, 1, &Materials);
    Context->PSSetShaderResources(1, 1, &Materials);

    for (const FMeshDraw& Draw : Draws) {
        const FMeshDrawBatch& Item{Draw.mBatch};
        const FMeshDrawState& State{Item.mState};

        Draw.mPipeline->Bind(Context, Draw.mMode, Draw.mStencilReference);
        Context->VSSetShaderResources(3, static_cast<UINT>(Draw.mTextures.size()), Draw.mTextures.data());
        Context->PSSetShaderResources(3, static_cast<UINT>(Draw.mTextures.size()), Draw.mTextures.data());

        ID3D11Buffer* Vertices[]{Draw.mMesh->GetVertexBuffer(EVertexAttribute::Position, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::Normal, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::UV, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::Color, State.mLODLevel)};
        constexpr Uint32 Offsets[]{0, 0, 0, 0};
        Uint32 DrawCount{};

        Context->IASetVertexBuffers(0, 4, Vertices, Draw.mVertexStrides.data(), Offsets);
        Context->IASetIndexBuffer(Draw.mMesh->GetIndexBuffer(State.mLODLevel), DXGI_FORMAT_R32_UINT, 0);

#if ENABLE_INSTANCE
        Context->DrawIndexedInstanced(State.mIndexCount, Item.mRecordCount, State.mFirstIndex, 0, Item.mFirstRecord);
        DrawCount = 1;
#else
        for (Uint32 Index{}; Index < Item.mRecordCount; ++Index) {
            if (mCurrentFrameResource->BindMeshDraw(Context, Item.mFirstRecord + Index)) {
                Context->DrawIndexed(State.mIndexCount, State.mFirstIndex, 0);
                ++DrawCount;
            }
        }
#endif

        Stat::RecordLODStats(State.mLODLevel, static_cast<std::uint64_t>(State.mIndexCount / 3) * Item.mRecordCount, static_cast<std::uint64_t>(State.mOriginalIndexCount / 3) * Item.mRecordCount, DrawCount);
    }
}

void FOverLayRenderer::DrawSceneGuides(ID3D11DeviceContext* Context, const FRenderView& View) {
    mLineRenderer.Clear();

    for (const FLineProbe& Line : View.mSceneGuides.GetLines()) {
        mLineRenderer.AddGridLine(Line.mStart, Line.mEnd, Line.mColor, Line.mWidthPixels, Line.mGridSpacing, Line.mDepthMode);
    }

    if (!mLineRenderer.IsEmpty()) {
        mLineRenderer.Render(Context, *mCurrentFrameResource);
    }
}

void FOverLayRenderer::RenderOrientationAxis(ID3D11DeviceContext* Context, const FRenderView& View, const FSceneRenderOutput& Output) {
    if (!View.IsPassEnabled(ERenderPass::OrientationAxis) || Context == nullptr || mCurrentFrameResource == nullptr || mCurrentDepth == nullptr || mCurrentTarget != Output.mTarget || !Output.IsValid()) {
        return;
    }

    constexpr float Margin{5.0f};
    const D3D11_VIEWPORT& Viewport{Output.mViewport};
    const float AvailableSize{std::min(Viewport.Width, Viewport.Height) - Margin * 2.0f};

    if (AvailableSize <= 0.0f) {
        return;
    }

    const float RequestedSize{View.mOrientationAxisSize > 0.0f ? View.mOrientationAxisSize : std::min(std::min(Viewport.Width, Viewport.Height) * 0.15f, 160.0f)};
    const float AxisSize{std::min(RequestedSize, AvailableSize)};
    const D3D11_VIEWPORT AxisViewport{Viewport.TopLeftX + Margin, Viewport.TopLeftY + Margin, AxisSize, AxisSize, Viewport.MinDepth, Viewport.MaxDepth};

    Output.mTarget->Bind(Context, mCurrentDepth);
    Context->RSSetViewports(1, &AxisViewport);

    FMatrix AxisView{View.mCamera.mView};

    AxisView.Translation(FVector3{0.0f, 0.0f, 3.0f});

    const FMatrix Projection{FMatrix::CreateOrthographic(2.5f, 2.5f, 0.5f, 10.0f)};

    mLineRenderer.Clear();
    mLineRenderer.AddRay(FVector3{}, FVector3{1.0f, 0.0f, 0.0f}, 1.0f, FVector4{1.0f, 0.0f, 0.0f, 1.0f}, 3.0f);
    mLineRenderer.AddRay(FVector3{}, FVector3{0.0f, 1.0f, 0.0f}, 1.0f, FVector4{0.0f, 0.0f, 1.0f, 1.0f}, 3.0f);
    mLineRenderer.AddRay(FVector3{}, FVector3{0.0f, 0.0f, 1.0f}, 1.0f, FVector4{0.0f, 0.0f, 1.0f, 1.0f}, 3.0f);

    const CameraProbe AxisCamera{AxisView * Projection, AxisView, Projection};

    if (mCurrentFrameResource->PrepareOrientationAxis(Context, AxisCamera, AxisViewport)) {
        mLineRenderer.RenderOrientationAxis(Context, *mCurrentFrameResource);
    }
}
