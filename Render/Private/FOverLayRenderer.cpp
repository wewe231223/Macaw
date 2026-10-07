#include "pch.h"
#include "Render/FOverLayRenderer.h"
#include "Asset/UMesh.h"
#include "Asset/UMaterial.h"
#include "Render/FLODSelection.h"
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

    UPipeline MaskPipeline{};
    UPipeline OutlinePipeline{};

    if (!MaskPipeline.Initialize("./Content/Pipeline/SelectionMask.json") || !OutlinePipeline.Initialize("./Content/Pipeline/SelectionOutline.json") || !mSelectionMaskPipeline.Initialize(Device, MaskPipeline) || !mSelectionOutlinePipeline.Initialize(Device, OutlinePipeline) || !mTextRenderer.Initialize(Device)) {
        Reset();
        return false;
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
    mObjectTransforms.clear();
    mDrawRecords.clear();
    mOutlineItems.clear();
    mGizmoItems.clear();
    mTemplates.clear();
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

bool FOverLayRenderer::BeginFrame(ID3D11DeviceContext* Context, Uint32 FrameResourceIndex, Uint64 FrameSerial) {
    if (mCurrentFrameResource != nullptr || FrameResourceIndex >= mFrameResources.size()) {
        return false;
    }

    FFrameResource& FrameResource{mFrameResources[FrameResourceIndex]};

    if (!FrameResource.BeginFrame(Context)) {
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
    mObjectTransforms.clear();
    mDrawRecords.clear();
    mOutlineItems.clear();
    mGizmoItems.clear();
    mTemplates.clear();
    mViewDepths.clear();
    mLineRenderer.Reset();
    mTextRenderer.Reset();
    mSelectionMaskPipeline.Reset();
    mSelectionOutlinePipeline.Reset();
    mFrameResources.clear();
    mAssetResources.Reset();
    mAssetRegistry = nullptr;
    mDevice = nullptr;
    mFrameSerial = 0;
}

ID3D11DepthStencilView* FOverLayRenderer::RenderView(ID3D11DeviceContext* Context, const FRenderView& View, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output) {
    mCurrentDepth = nullptr;
    mCurrentTarget = nullptr;
    mOutlineDraws.clear();
    mGizmoDraws.clear();
    mObjectTransforms.clear();
    mDrawRecords.clear();
    mOutlineItems.clear();
    mGizmoItems.clear();

    if (Context == nullptr || mDevice == nullptr || mCurrentFrameResource == nullptr || !Output.IsValid() || Overlay.mPasses.none()) {
        return Output.mDepthStencilView;
    }

    if (mAssetRegistry != nullptr) {
        if (!mAssetResources.GetMaterialBuffer().Synchronize(*mAssetRegistry, Context)) {
            return Output.mDepthStencilView;
        }

        if (Overlay.IsPassEnabled(EOverlayPass::SelectionOutline)) {
            BuildMeshItems(Overlay.mSelectionProbes, View, Output.mViewport, true, mOutlineItems);
        }

        if (Overlay.IsPassEnabled(EOverlayPass::Gizmo)) {
            BuildMeshItems(Overlay.mGizmoProbes, View, Output.mViewport, false, mGizmoItems);
        }
    }

    if (!PrepareMeshDraws(mOutlineItems, true, mOutlineDraws) || !PrepareMeshDraws(mGizmoItems, false, mGizmoDraws) || !mCurrentFrameResource->PrepareView(mDevice, Context, View.mCamera, Output.mViewport, Overlay.mGridFade, mObjectTransforms, mDrawRecords) || !PrepareDepth(Context, Output)) {
        return Output.mDepthStencilView;
    }

    Output.mTarget->Bind(Context, mCurrentDepth);

    if (!mOutlineDraws.empty() && PrepareSelectionMask()) {
        DrawSelectionOutline(Context, Output);
    }

    if (Overlay.IsPassEnabled(EOverlayPass::Guides)) {
        DrawGuides(Context, Overlay);
    }

    if (!mGizmoDraws.empty()) {
        Context->ClearDepthStencilView(mCurrentDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
        DrawMeshes(Context, mGizmoDraws);
    }

    if (Overlay.IsPassEnabled(EOverlayPass::Text) && mAssetRegistry != nullptr) {
        mTextRenderer.Render(Context, *mCurrentFrameResource, View.mCamera, Output.mViewport, Overlay.mTextProbes, *mAssetRegistry, mAssetResources);
    }

    RenderOrientationAxis(Context, View.mCamera, Overlay, Output);
    Output.mTarget->Bind(Context, mCurrentDepth);

    return mCurrentDepth;
}

void FOverLayRenderer::BuildMeshItems(const TArray<FActorProbe>& Probes, const FRenderView& View, const D3D11_VIEWPORT& Viewport, bool Selection, TArray<FMeshDrawBatch>& Items) {
    for (const FActorProbe& Probe : Probes) {
        const UMesh* Mesh{mAssetRegistry->ResolveAsset<UMesh>(Probe.mMeshHandle)};
        const UMaterial* Material{mAssetRegistry->ResolveAsset<UMaterial>(Probe.mMaterialHandle)};

        if (Mesh == nullptr || Material == nullptr) {
            continue;
        }

        FLODSelection LOD{};

        if (Selection && View.mUseLOD) {
            const DirectX::BoundingSphere& Bounds{Probe.mWorldSphereBounds};
            float ScreenSize{Bounds.Radius * std::abs(View.mCamera.mProjection.M[1][1])};

            if (std::abs(View.mCamera.mProjection.M[2][3]) > 1e-6f) {
                const FMatrix& CameraView{View.mCamera.mView};
                const float Depth{Bounds.Center.x * CameraView.M[0][2] + Bounds.Center.y * CameraView.M[1][2] + Bounds.Center.z * CameraView.M[2][2] + CameraView.M[3][2]};

                ScreenSize /= std::max(std::abs(Depth), 1e-4f);
            }

            Uint32 AvailableMask{};

            for (Uint32 Level{}; Level < GLODCount; ++Level) {
                if (Mesh->HasLOD(Level)) {
                    AvailableMask |= 1u << Level;
                }
            }

            LOD = SelectMeshLOD(ScreenSize, Viewport.Height, AvailableMask, true);
        }

        if (LOD.mCulled || mObjectTransforms.size() >= UINT32_MAX) {
            continue;
        }

        const Uint32 ObjectIndex{static_cast<Uint32>(mObjectTransforms.size())};

        mObjectTransforms.push_back(Probe.mWorld);

        auto AddLevel{[&](Uint32 Level, float Dither) {
            mTemplates.clear();
            AppendMeshDrawTemplates(*Mesh, *Material, mAssetResources.GetMaterialBuffer(), Probe.mPipelineHandle, Probe.mMeshHandle, Level, mTemplates);

            for (const FRenderBatchTemplate& Template : mTemplates) {
                if (mDrawRecords.size() >= UINT32_MAX) {
                    break;
                }

                const Uint32 FirstRecord{static_cast<Uint32>(mDrawRecords.size())};

                mDrawRecords.push_back(FMeshDrawRecord{ObjectIndex, Template.mMaterialIndex, Probe.mFlags, Dither});
                Items.push_back(FMeshDrawBatch{Template.mState, FirstRecord, 1, Probe.mFlags});
            }
        }};

        AddLevel(LOD.mLevel, LOD.mDither);

        if (LOD.mNextLevel != UINT32_MAX) {
            AddLevel(LOD.mNextLevel, -LOD.mDither);
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

void FOverLayRenderer::DrawSelectionOutline(ID3D11DeviceContext* Context, const FSceneRenderOutput& Output) {
    FViewDepth& Depth{mViewDepths[mCurrentTarget]};
    constexpr float ClearColor[]{0.0f, 0.0f, 0.0f, 0.0f};

    Context->ClearRenderTargetView(Depth.mSelectionMaskTarget.Get(), ClearColor);
    Context->OMSetRenderTargets(1, Depth.mSelectionMaskTarget.GetAddressOf(), mCurrentDepth);
    Context->RSSetViewports(1, &Output.mViewport);
    DrawMeshes(Context, mOutlineDraws, true);
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
        const UPipeline* Pipeline{mAssetRegistry->ResolveAsset<UPipeline>(Item.mState.mPipelineHandle)};
        const UMesh* Mesh{mAssetRegistry->ResolveAsset<UMesh>(Item.mState.mMeshHandle)};

        if (Mesh == nullptr || Item.mRecordCount == 0 || (!Selection && Pipeline == nullptr)) {
            continue;
        }

        FMeshDraw Draw{};

        Draw.mMode = Selection ? ERenderMode::Lit : Pipeline->ResolveRenderMode(ERenderMode::Lit);

        if (!Selection && !Pipeline->RenderModeSettable(Draw.mMode)) {
            continue;
        }

        Draw.mMesh = mAssetResources.GetMesh(*Mesh);
        Draw.mPipeline = Selection ? &mSelectionOutlinePipeline : mAssetResources.GetPipeline(*Pipeline);

        if (Draw.mMesh == nullptr || Draw.mPipeline == nullptr) {
            return false;
        }

        Draw.mBatch = Item;
        Draw.mStencilReference = Selection ? 1u : 0u;
        Draw.mVertexStrides = {Mesh->GetVertexStride(EVertexAttribute::Position), Mesh->GetVertexStride(EVertexAttribute::Normal), Mesh->GetVertexStride(EVertexAttribute::UV), Mesh->GetVertexStride(EVertexAttribute::Color)};

        for (Uint8 Index{}; Index < Item.mState.mTextureSignature.mTextureFieldCount; ++Index) {
            const UTexture* Texture{mAssetRegistry->ResolveAsset<UTexture>(Item.mState.mTextureSignature.GetTextureHandle(Index))};

            Draw.mTextures[Index] = Texture != nullptr ? mAssetResources.GetTexture(*Texture) : nullptr;
        }

        Draws.push_back(Draw);
    }

    return true;
}

void FOverLayRenderer::DrawMeshes(ID3D11DeviceContext* Context, const TArray<FMeshDraw>& Draws, bool SelectionMask) {
    if (Draws.empty() || !mCurrentFrameResource->BindModels(Context)) {
        return;
    }

    ID3D11ShaderResourceView* Materials{mAssetResources.GetMaterialBuffer().GetSRV()};

    Context->VSSetShaderResources(1, 1, &Materials);
    Context->PSSetShaderResources(1, 1, &Materials);

    for (const FMeshDraw& Draw : Draws) {
        const FMeshDrawBatch& Item{Draw.mBatch};
        const FMeshDrawState& State{Item.mState};

        const FPipelineRenderResource* Pipeline{SelectionMask ? &mSelectionMaskPipeline : Draw.mPipeline};

        Pipeline->Bind(Context, Draw.mMode, Draw.mStencilReference);

        Context->VSSetShaderResources(3, static_cast<UINT>(Draw.mTextures.size()), Draw.mTextures.data());
        Context->PSSetShaderResources(3, static_cast<UINT>(Draw.mTextures.size()), Draw.mTextures.data());

        ID3D11Buffer* Vertices[]{Draw.mMesh->GetVertexBuffer(EVertexAttribute::Position, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::Normal, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::UV, State.mLODLevel), Draw.mMesh->GetVertexBuffer(EVertexAttribute::Color, State.mLODLevel)};
        constexpr Uint32 Offsets[]{0, 0, 0, 0};

        Context->IASetVertexBuffers(0, 4, Vertices, Draw.mVertexStrides.data(), Offsets);

        Context->DrawInstanced(State.mIndexCount, Item.mRecordCount, State.mFirstIndex, Item.mFirstRecord);

        Stat::RecordLODStats(State.mLODLevel, static_cast<std::uint64_t>(State.mIndexCount / 3) * Item.mRecordCount, static_cast<std::uint64_t>(State.mOriginalIndexCount / 3) * Item.mRecordCount, 1);
    }
}

void FOverLayRenderer::DrawGuides(ID3D11DeviceContext* Context, const FOverlayRenderData& Overlay) {
    mLineRenderer.Clear();

    for (const FLineProbe& Line : Overlay.mGuides.GetLines()) {
        mLineRenderer.AddGridLine(Line.mStart, Line.mEnd, Line.mColor, Line.mWidthPixels, Line.mGridSpacing, Line.mDepthMode);
    }

    if (!mLineRenderer.IsEmpty()) {
        mLineRenderer.Render(Context, *mCurrentFrameResource);
    }
}

void FOverLayRenderer::RenderOrientationAxis(ID3D11DeviceContext* Context, const CameraProbe& Camera, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output) {
    if (!Overlay.IsPassEnabled(EOverlayPass::OrientationAxis) || Context == nullptr || mCurrentFrameResource == nullptr || mCurrentDepth == nullptr || mCurrentTarget != Output.mTarget || !Output.IsValid()) {
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

    const CameraProbe AxisCamera{AxisView * Projection, AxisView, Projection};

    if (mCurrentFrameResource->PrepareOrientationAxis(Context, AxisCamera, AxisViewport)) {
        mLineRenderer.RenderOrientationAxis(Context, *mCurrentFrameResource);
    }
}
