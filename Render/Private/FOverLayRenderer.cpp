#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Render/FOverLayRenderer.h"
#include "Asset/UMesh.h"
#include "Asset/UMaterial.h"
#include "Asset/UTexture.h"
#include "Core/Stat/Stat.h"

bool FOverLayRenderer::Initialize(ID3D11Device* Device) {
    Reset();

    if (Device == nullptr) {
        return false;
    }

    UPipeline MaskPipeline{};
    UPipeline OutlinePipeline{};

    if (!MaskPipeline.Initialize("./Content/Pipeline/SelectionMask.json") || !OutlinePipeline.Initialize("./Content/Pipeline/SelectionOutline.json") || !mSelectionMaskPipeline.Initialize(Device, MaskPipeline) || !mSelectionOutlinePipeline.Initialize(Device, OutlinePipeline) || !mTextRenderer.Initialize(Device) || !mAssetResources.Initialize(Device, false)) {
        Reset();
        return false;
    }

    mLineRenderer.Initialize(Device);
    mDevice = Device;

    return true;
}

void FOverLayRenderer::BeginFrame(Uint64 FrameSerial) {
    mFrameSerial = FrameSerial;
    mTextRenderer.BeginFrame(FrameSerial);
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;
    mOutlineDraws.clear();
    mGizmoDraws.clear();

    constexpr Uint64 MaximumUnusedFrames{120};
    std::erase_if(mViewDepths, [FrameSerial](const auto& Entry) {
        return FrameSerial - Entry.second.mLastUsedFrame > MaximumUnusedFrames;
    });

    mAssetResources.Prune();
}

void FOverLayRenderer::Reset() {
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;
    mOutlineDraws.clear();
    mGizmoDraws.clear();
    mObjectTransforms.clear();
    mDrawRecords.clear();
    mOutlineItems.clear();
    mGizmoItems.clear();
    mViewDepths.clear();
    mLineRenderer.Reset();
    mTextRenderer.Reset();
    mSelectionMaskPipeline.Reset();
    mSelectionOutlinePipeline.Reset();
    mAssetResources.Reset();
    mDevice = nullptr;
    mFrameSerial = 0;
}

void FOverLayRenderer::RenderView(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FRenderView& View, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output) {
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;
    mOutlineDraws.clear();
    mGizmoDraws.clear();
    mObjectTransforms.clear();
    mDrawRecords.clear();
    mOutlineItems.clear();
    mGizmoItems.clear();

    if (Context == nullptr || mDevice == nullptr || !Output.IsValid() || Overlay.mPasses.none()) {
        return;
    }

    if (FAssetRegistry::TryGet() != nullptr) {
        if (!mAssetResources.GetMaterialBuffer().Synchronize(Context)) {
            return;
        }

        if (Overlay.IsPassEnabled(EOverlayPass::SelectionOutline)) {
            BuildMeshItems(Overlay.mSelectionMeshes, mOutlineItems);
        }

        if (Overlay.IsPassEnabled(EOverlayPass::Gizmo)) {
            BuildMeshItems(Overlay.mGizmoMeshes, mGizmoItems);
        }
    }

    if (!PrepareMeshDraws(mOutlineItems, true, mOutlineDraws) || !PrepareMeshDraws(mGizmoItems, false, mGizmoDraws) || !RenderBuffers.PrepareView(mDevice, Context, View.mCamera, Output.mViewport, Overlay.mGridFade, mObjectTransforms, mDrawRecords) || !PrepareDepth(Context, Output)) {
        return;
    }

    Output.mTarget->Bind(Context, mCurrentDepth);

    if (!mOutlineDraws.empty() && PrepareSelectionMask()) {
        DrawSelectionOutline(Context, RenderBuffers, Output);
    }

    if (Overlay.IsPassEnabled(EOverlayPass::Guides)) {
        DrawGuides(Context, RenderBuffers, Overlay);
    }

    if (Overlay.IsPassEnabled(EOverlayPass::Gizmo) && (!mGizmoDraws.empty() || !Overlay.mGizmoLines.GetLines().empty())) {
        Context->ClearDepthStencilView(mCurrentDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
        DrawMeshes(Context, RenderBuffers, mGizmoDraws);
        mLineRenderer.Clear();

        for (const FLineDrawData& Line : Overlay.mGizmoLines.GetLines()) {
            mLineRenderer.AddLine(Line.mStart, Line.mEnd, Line.mColor, Line.mWidthPixels, ELineDepthMode::Overlay);
        }

        if (!mLineRenderer.IsEmpty()) {
            mLineRenderer.Render(Context, RenderBuffers);
        }
    }

    if (Overlay.IsPassEnabled(EOverlayPass::Text) && FAssetRegistry::TryGet() != nullptr) {
        mTextRenderer.Render(Context, RenderBuffers, View.mCamera, Output.mViewport, Overlay.mTextDraws, mAssetResources);
    }

    RenderOrientationAxis(Context, RenderBuffers, View.mCamera, Overlay, Output);
    Output.mTarget->Bind(Context, mCurrentDepth);

    RenderBuffers.BindCommon(Context);
}

void FOverLayRenderer::BuildMeshItems(const TArray<FOverlayMeshDrawData>& Meshes, TArray<FMeshDrawBatch>& Items) {
    for (const FOverlayMeshDrawData& Data : Meshes) {
        const UMesh* Mesh{FAssetRegistry::Get().ResolveAsset<UMesh>(Data.mMesh.mMeshHandle)};
        const UMaterial* Material{FAssetRegistry::Get().ResolveAsset<UMaterial>(Data.mMesh.mMaterialHandle)};

        if (Mesh == nullptr || Material == nullptr || mObjectTransforms.size() >= UINT32_MAX) {
            continue;
        }

        const Uint32 ObjectIndex{static_cast<Uint32>(mObjectTransforms.size())};

        mObjectTransforms.push_back(Data.mWorld);

        FMeshBatch Batch{};

        Batch.mMeshHandle = Data.mMesh.mMeshHandle;
        Batch.mMaterialHandle = Data.mMesh.mMaterialHandle;
        Batch.mPipelineHandle = Data.mMesh.mPipelineHandle;

        BuildMeshBatchElements(*Mesh, 0, Batch.mElements);

        for (const FMeshBatchElement& Element : Batch.mElements) {
            FMeshDrawState State{};
            Uint32 MaterialIndex{UINT32_MAX};

            if (mDrawRecords.size() >= UINT32_MAX || !BuildMeshDrawState(Batch, Element, *Material, mAssetResources.GetMaterialBuffer(), State, MaterialIndex)) {
                continue;
            }

            const Uint32 FirstRecord{static_cast<Uint32>(mDrawRecords.size())};

            mDrawRecords.push_back(FMeshDrawRecord{ObjectIndex, MaterialIndex});
            Items.push_back(FMeshDrawBatch{State, FirstRecord, 1});
        }
    }
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

bool FOverLayRenderer::PrepareSelectionMask() {
    FViewDepth& Depth{mViewDepths[mCurrentTarget]};

    if (Depth.mSelectionMaskTarget != nullptr && Depth.mSelectionMaskResource != nullptr) {
        return true;
    }

    D3D11_TEXTURE2D_DESC Description{};

    Depth.mTexture->GetDesc(&Description);
    Description.Format = DXGI_FORMAT_R8_UNORM;
    Description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    return SUCCEEDED(mDevice->CreateTexture2D(&Description, nullptr, Depth.mSelectionMask.ReleaseAndGetAddressOf())) && SUCCEEDED(mDevice->CreateRenderTargetView(Depth.mSelectionMask.Get(), nullptr, Depth.mSelectionMaskTarget.ReleaseAndGetAddressOf())) && SUCCEEDED(mDevice->CreateShaderResourceView(Depth.mSelectionMask.Get(), nullptr, Depth.mSelectionMaskResource.ReleaseAndGetAddressOf()));
}

void FOverLayRenderer::DrawSelectionOutline(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FSceneRenderOutput& Output) {
    FViewDepth& Depth{mViewDepths[mCurrentTarget]};
    constexpr float ClearColor[]{0.0f, 0.0f, 0.0f, 0.0f};

    Context->ClearRenderTargetView(Depth.mSelectionMaskTarget.Get(), ClearColor);
    Context->OMSetRenderTargets(1, Depth.mSelectionMaskTarget.GetAddressOf(), mCurrentDepth);
    Context->RSSetViewports(1, &Output.mViewport);
    DrawMeshes(Context, RenderBuffers, mOutlineDraws);
    Output.mTarget->Bind(Context, mCurrentDepth);
    mSelectionOutlinePipeline.Bind(Context, ERenderMode::Lit);
    Context->PSSetShaderResources(3, 1, Depth.mSelectionMaskResource.GetAddressOf());
    Context->DrawInstanced(3, 1, 0, 0);

    ID3D11ShaderResourceView* NullResource{nullptr};

    Context->PSSetShaderResources(3, 1, &NullResource);
}

bool FOverLayRenderer::PrepareMeshDraws(const TArray<FMeshDrawBatch>& Items, bool Selection, TArray<FMeshDraw>& Draws) {
    Draws.clear();
    Draws.reserve(Items.size());

    for (const FMeshDrawBatch& Item : Items) {
        const UPipeline* Pipeline{FAssetRegistry::Get().ResolveAsset<UPipeline>(Item.mState.mPipelineHandle)};
        const UMesh* Mesh{FAssetRegistry::Get().ResolveAsset<UMesh>(Item.mState.mMeshHandle)};

        if (Mesh == nullptr || Item.mRecordCount == 0 || (!Selection && Pipeline == nullptr)) {
            continue;
        }

        FMeshDraw Draw{};

        Draw.mMode = Selection ? ERenderMode::Lit : Pipeline->ResolveRenderMode(ERenderMode::Lit);

        if (!Selection && !Pipeline->RenderModeSettable(Draw.mMode)) {
            continue;
        }

        Draw.mMesh = mAssetResources.GetMesh(*Mesh);
        Draw.mPipeline = Selection ? &mSelectionMaskPipeline : mAssetResources.GetPipeline(*Pipeline);

        if (Draw.mMesh == nullptr || Draw.mPipeline == nullptr) {
            return false;
        }

        Draw.mBatch = Item;
        Draw.mVertexStrides = {Mesh->GetVertexStride(EVertexAttribute::Position), Mesh->GetVertexStride(EVertexAttribute::Normal), Mesh->GetVertexStride(EVertexAttribute::UV), Mesh->GetVertexStride(EVertexAttribute::Color)};

        for (Uint8 Index{}; Index < Item.mState.mTextureSignature.mTextureFieldCount; ++Index) {
            const UTexture* Texture{FAssetRegistry::Get().ResolveAsset<UTexture>(Item.mState.mTextureSignature.GetTextureHandle(Index))};

            Draw.mTextures[Index] = Texture != nullptr ? mAssetResources.GetTexture(*Texture) : nullptr;
        }

        Draws.push_back(Draw);
    }

    return true;
}

void FOverLayRenderer::DrawMeshes(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const TArray<FMeshDraw>& Draws) {
    if (Draws.empty() || !RenderBuffers.BindModels(Context)) {
        return;
    }

    ID3D11ShaderResourceView* Materials{mAssetResources.GetMaterialBuffer().GetSRV()};

    Context->VSSetShaderResources(1, 1, &Materials);
    Context->PSSetShaderResources(1, 1, &Materials);

    for (const FMeshDraw& Draw : Draws) {
        const FMeshDrawBatch& Item{Draw.mBatch};
        const FMeshDrawState& State{Item.mState};

        Draw.mPipeline->Bind(Context, Draw.mMode);

        Context->VSSetShaderResources(3, static_cast<UINT>(Draw.mTextures.size()), Draw.mTextures.data());
        Context->PSSetShaderResources(3, static_cast<UINT>(Draw.mTextures.size()), Draw.mTextures.data());

        ID3D11Buffer* Vertices[]{Draw.mMesh->GetVertexBuffer(EVertexAttribute::Position, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::Normal, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::UV, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::Color, State.mLODLevel)};
        constexpr Uint32 Offsets[]{0, 0, 0, 0};

        Context->IASetVertexBuffers(0, 4, Vertices, Draw.mVertexStrides.data(), Offsets);

        Context->DrawInstanced(State.mIndexCount, Item.mRecordCount, State.mFirstIndex, Item.mFirstRecord);

        Stat::RecordLODStats(State.mLODLevel, static_cast<std::uint64_t>(State.mIndexCount / 3) * Item.mRecordCount, static_cast<std::uint64_t>(State.mOriginalIndexCount / 3) * Item.mRecordCount, 1);
    }
}

void FOverLayRenderer::DrawGuides(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FOverlayRenderData& Overlay) {
    mLineRenderer.Clear();

    for (const FLineDrawData& Line : Overlay.mGuides.GetLines()) {
        mLineRenderer.AddGridLine(Line.mStart, Line.mEnd, Line.mColor, Line.mWidthPixels, Line.mGridSpacing, Line.mDepthMode);
    }

    if (!mLineRenderer.IsEmpty()) {
        mLineRenderer.Render(Context, RenderBuffers);
    }
}

void FOverLayRenderer::RenderOrientationAxis(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FViewMatrices& Camera, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output) {
    if (!Overlay.IsPassEnabled(EOverlayPass::OrientationAxis) || Context == nullptr || mCurrentDepth == nullptr || mCurrentTarget != Output.mTarget || !Output.IsValid()) {
        return;
    }

    constexpr float Margin{5.0f};
    const D3D11_VIEWPORT& Viewport{Output.mViewport};
    const float AvailableSize{std::min(Viewport.Width, Viewport.Height) - Margin * 2.0f};

    if (AvailableSize <= 0.0f) {
        return;
    }

    const float RequestedSize{Overlay.mOrientationAxisSize > 0.0f ? Overlay.mOrientationAxisSize : std::min(std::min(Viewport.Width, Viewport.Height) * 0.15f, 160.0f)};
    const float AxisSize{std::min(RequestedSize, AvailableSize)};
    const D3D11_VIEWPORT AxisViewport{Viewport.TopLeftX + Margin, Viewport.TopLeftY + Margin, AxisSize, AxisSize, Viewport.MinDepth, Viewport.MaxDepth};

    Output.mTarget->Bind(Context, mCurrentDepth);
    Context->RSSetViewports(1, &AxisViewport);

    FMatrix AxisView{Camera.mView};

    AxisView.Translation(FVector3{0.0f, 0.0f, 3.0f});

    const FMatrix Projection{FMatrix::CreateOrthographic(2.5f, 2.5f, 0.5f, 10.0f)};

    mLineRenderer.Clear();
    mLineRenderer.AddRay(FVector3{}, FVector3{1.0f, 0.0f, 0.0f}, 1.0f, FVector4{1.0f, 0.0f, 0.0f, 1.0f}, 3.0f);
    mLineRenderer.AddRay(FVector3{}, FVector3{0.0f, 1.0f, 0.0f}, 1.0f, FVector4{0.0f, 1.0f, 0.0f, 1.0f}, 3.0f);
    mLineRenderer.AddRay(FVector3{}, FVector3{0.0f, 0.0f, 1.0f}, 1.0f, FVector4{0.0f, 0.0f, 1.0f, 1.0f}, 3.0f);

    const FViewMatrices AxisCamera{AxisView * Projection, AxisView, Projection};

    if (RenderBuffers.PrepareOrientationAxis(Context, AxisCamera, AxisViewport)) {
        mLineRenderer.RenderOrientationAxis(Context, RenderBuffers);
    }
}
