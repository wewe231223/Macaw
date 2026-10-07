#pragma once

#include "Render/FFrameResource.h"
#include "Render/FLineRenderer.h"
#include "Render/FRenderAssetResources.h"
#include "Render/FRenderContext.h"
#include "Render/FSceneRenderOutput.h"

class FOverLayRenderer {
private:
    struct FMeshDraw {
        FMeshDrawBatch mBatch{};
        const FMeshRenderResource* mMesh{nullptr};
        const FPipelineRenderResource* mPipeline{nullptr};
        std::array<ID3D11ShaderResourceView*, MaxMaterialTextureFields> mTextures{};
        std::array<Uint32, 4> mVertexStrides{};
        ERenderMode mMode{ERenderMode::Lit};
        Uint32 mStencilReference{};
    };

    struct FViewDepth {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> mTexture{};
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mView{};
        Uint64 mLastUsedFrame{};
    };

public:
    bool Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, Uint32 FrameResourceCount);
    bool BindAssetRegistry(const IAssetRegistry* Registry);
    bool BeginFrame(ID3D11DeviceContext* Context, Uint32 FrameResourceIndex, Uint64 FrameSerial, float AnimationTime);
    void EndFrame();
    void Reset();

    ID3D11DepthStencilView* RenderView(const FRenderContext& Context, const FRenderView& View, const FRenderScene& Scene, const FRenderQueue& SceneQueue, const FSceneRenderOutput& Output);
    void RenderOrientationAxis(ID3D11DeviceContext* Context, const FRenderView& View, const FSceneRenderOutput& Output);

private:
    bool PrepareDepth(ID3D11DeviceContext* Context, const FSceneRenderOutput& Output);
    bool PrepareMeshDraws(const TArray<FMeshDrawBatch>& Items, ERenderMode Mode, TArray<FMeshDraw>& Draws);
    void DrawMeshes(ID3D11DeviceContext* Context, const TArray<FMeshDraw>& Draws);
    void DrawSceneGuides(ID3D11DeviceContext* Context, const FRenderView& View);

private:
    ID3D11Device* mDevice{nullptr};
    const IAssetRegistry* mAssetRegistry{nullptr};
    FRenderAssetResources mAssetResources{};
    TArray<FFrameResource> mFrameResources{};
    FFrameResource* mCurrentFrameResource{nullptr};
    Uint64 mFrameSerial{};
    FRenderQueue mQueue{};
    TArray<FMeshDraw> mOutlineDraws{};
    TArray<FMeshDraw> mGizmoDraws{};
    TMap<const IRenderSurface*, FViewDepth> mViewDepths{};
    const IRenderSurface* mCurrentTarget{nullptr};
    ID3D11DepthStencilView* mCurrentDepth{nullptr};
    FLineRenderer mLineRenderer{};
};
