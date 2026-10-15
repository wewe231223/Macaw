/**
 * @file FOverLayRenderer.h
 * @brief 전역 에셋을 조회하며 뷰포트별 깊이와 선택 마스크를 유지하는 오버레이 렌더러를 정의합니다.
 */
#pragma once

#include "Render/FRenderBuffers.h"
#include "Render/FLineRenderer.h"
#include "Render/FRenderAssetResources.h"
#include "Render/FSceneRenderOutput.h"
#include "Render/FOverlayTextRenderer.h"
#include "RenderCore/FOverlayRenderData.h"

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
    /**
     * @brief 오버레이 파이프라인과 렌더러 소유 GPU 에셋 캐시를 생성합니다.
     * @param Device 사용할 그래픽 장치입니다.
     * @return 모든 자원 생성에 성공하면 true입니다.
     */
    bool Initialize(ID3D11Device* Device);
    void BeginFrame(Uint64 FrameSerial);
    void Reset();

    void RenderView(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FRenderView& View, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output);

private:
    void BuildMeshItems(const TArray<FOverlayMeshDrawData>& Meshes, TArray<FMeshDrawBatch>& Items);
    bool PrepareDepth(ID3D11DeviceContext* Context, const FSceneRenderOutput& Output);
    bool PrepareSelectionMask();
    void DrawSelectionOutline(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FSceneRenderOutput& Output);
    bool PrepareMeshDraws(const TArray<FMeshDrawBatch>& Items, bool Selection, TArray<FMeshDraw>& Draws);
    void DrawMeshes(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const TArray<FMeshDraw>& Draws);
    void DrawGuides(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FOverlayRenderData& Overlay);
    void RenderOrientationAxis(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FViewMatrices& Camera, const FOverlayRenderData& Overlay, const FSceneRenderOutput& Output);

private:
    ID3D11Device* mDevice{nullptr};
    FRenderAssetResources mAssetResources{};
    Uint64 mFrameSerial{};
    TArray<FMatrix> mObjectTransforms{};
    TArray<FMeshDrawRecord> mDrawRecords{};
    TArray<FMeshDrawBatch> mOutlineItems{};
    TArray<FMeshDrawBatch> mGizmoItems{};
    TArray<FMeshDraw> mOutlineDraws{};
    TArray<FMeshDraw> mGizmoDraws{};
    TMap<const FSceneRenderSurface*, FViewDepth> mViewDepths{};
    const FSceneRenderSurface* mCurrentTarget{nullptr};
    ID3D11DepthStencilView* mCurrentDepth{nullptr};
    FLineRenderer mLineRenderer{};
    FOverlayTextRenderer mTextRenderer{};
    FPipelineRenderResource mSelectionMaskPipeline{};
    FPipelineRenderResource mSelectionOutlinePipeline{};
};
