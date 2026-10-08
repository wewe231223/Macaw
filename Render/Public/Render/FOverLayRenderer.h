#pragma once

#include "Render/FFrameResource.h"
#include "Render/FLineRenderer.h"
#include "Render/FRenderAssetResources.h"
#include "Render/FSceneRenderOutput.h"
#include "Render/FOverlayTextRenderer.h"
#include "RenderCore/FOverlayRenderData.h"
#include "Render/FStaticMeshBatch.h"

class FOverLayRenderer {
private:
    struct FMeshDraw {
        FMeshDrawBatch mBatch{};
        const FMeshRenderResource* mMesh{nullptr};
        const FPipelineRenderResource* mPipeline{nullptr};
        std::array<ID3D11ShaderResourceView*, MaxMaterialTextureFields> mTextures{};
        std::array<Uint32, 4> mVertexStrides{};
        ERenderMode mMode{ERenderMode::Lit};
    };

    struct FViewDepth {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> mTexture{};
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mView{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> mSelectionMask{};
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mSelectionMaskTarget{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSelectionMaskResource{};
        Uint64 mLastUsedFrame{};
    };

public:
    bool Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, Uint32 FrameResourceCount);
    bool BindAssetRegistry(const IAssetRegistry* Registry, IAssetRegistryMutator* Mutator = nullptr);
    bool BeginFrame(ID3D11DeviceContext* Context, Uint32 FrameResourceIndex, Uint64 FrameSerial);
    void EndFrame();
    void Reset();

    void RenderView(ID3D11DeviceContext* Context, const FRenderView& View, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output);

private:
    bool PrepareDepth(ID3D11DeviceContext* Context, const FSceneRenderOutput& Output);
    bool PrepareSelectionMask();
    void DrawSelectionOutline(ID3D11DeviceContext* Context, const FSceneRenderOutput& Output);
    void BuildMeshItems(const TArray<FActorProbe>& Probes, const FRenderView& View, const D3D11_VIEWPORT& Viewport, bool Selection, TArray<FMeshDrawBatch>& Items);
    bool PrepareMeshDraws(const TArray<FMeshDrawBatch>& Items, bool Selection, TArray<FMeshDraw>& Draws);
    void DrawMeshes(ID3D11DeviceContext* Context, const TArray<FMeshDraw>& Draws);
    void DrawGuides(ID3D11DeviceContext* Context, const FOverlayRenderData& Overlay);
    void RenderOrientationAxis(ID3D11DeviceContext* Context, const CameraProbe& Camera, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output);

private:
    ID3D11Device* mDevice{nullptr};
    const IAssetRegistry* mAssetRegistry{nullptr};
    FRenderAssetResources mAssetResources{};
    TArray<FFrameResource> mFrameResources{};
    FFrameResource* mCurrentFrameResource{nullptr};
    Uint64 mFrameSerial{};
    TArray<FMatrix> mObjectTransforms{};
    TArray<FMeshDrawRecord> mDrawRecords{};
    TArray<FMeshDrawBatch> mOutlineItems{};
    TArray<FMeshDrawBatch> mGizmoItems{};
    TArray<FStaticMeshBatch> mStaticMeshes{};
    TArray<FMeshDraw> mOutlineDraws{};
    TArray<FMeshDraw> mGizmoDraws{};
    TMap<const IRenderSurface*, FViewDepth> mViewDepths{};
    const IRenderSurface* mCurrentTarget{nullptr};
    ID3D11DepthStencilView* mCurrentDepth{nullptr};
    FLineRenderer mLineRenderer{};
    FOverlayTextRenderer mTextRenderer{};
    FPipelineRenderResource mSelectionMaskPipeline{};
    FPipelineRenderResource mSelectionOutlinePipeline{};
};
