/**
 * @file FAssetThumbnailRenderer.h
 * @brief 전역 에셋을 사용하는 썸네일과 프리뷰 렌더링을 정의합니다.
 */

#pragma once

#include <d3d11.h>
#include <cstddef>
#include <memory>
#include "Asset/FAssetEntry.h"
#include "Editor/View/FPreviewScene.h"
#include "Render/FOffScreenRenderSurface.h"
#include "Render/Renderer.h"
#include "Asset/UMesh.h"

class FAssetThumbnailRenderer {
private:
    struct FThumbnail {
        std::unique_ptr<FOffScreenRenderSurface> mSurface{};
    };

public:
    FAssetThumbnailRenderer() = default;
    ~FAssetThumbnailRenderer() = default;

    FAssetThumbnailRenderer(const FAssetThumbnailRenderer&) = delete;

    FAssetThumbnailRenderer& operator=(const FAssetThumbnailRenderer&) = delete;

    FAssetThumbnailRenderer(FAssetThumbnailRenderer&&) = delete;

    FAssetThumbnailRenderer& operator=(FAssetThumbnailRenderer&&) = delete;

public:
    /**
     * @brief 전역 레지스트리에서 썸네일 생성 대상을 수집합니다.
     * @param InRenderer 썸네일 표면을 생성하고 렌더링할 렌더러입니다.
     * @details 에셋 서비스 세대가 바뀌면 기존 썸네일과 프리뷰 자원을 해제합니다.
     * @note 렌더러 또는 활성 에셋 서비스가 없으면 생성 대상을 수집하지 않습니다.
     */
    void Create(FRenderer* InRenderer);
    void Tick(Uint32 MaxThumbnailCount = 1);
    float GetGenerationProgress() const;

    void RenderThumbnail(FAssetHandle AssetHandle);
    void RenderMaterialPreview(FAssetHandle MaterialHandle, FSceneRenderSurface& Surface);

    /**
     * @brief 현재 전역 에셋의 타입과 생존 여부를 검사하고 썸네일을 조회합니다.
     * @param AssetHandle 표시할 메시, 머티리얼 또는 텍스처 핸들입니다.
     * @return 썸네일의 비소유 포인터입니다. 에셋이나 렌더러가 없거나 서비스 세대가 변경되었으면 nullptr입니다.
     * @details 호출자는 에셋을 별도로 검증하지 않아도 됩니다. 텍스처는 렌더러의 GPU 자원을 반환합니다.
     */
    ID3D11ShaderResourceView* GetThumbnail(FAssetHandle AssetHandle) const;

    void Terminate();

private:
    void RenderThumbnail(const FAssetEntry& Entry, FSceneRenderSurface* PreviewSurface = nullptr);

    FMatrix BuildMeshTransform(const UMesh& Mesh) const;
    FViewMatrices BuildCamera() const;
    FMatrix MakeCameraWorldMatrix(const FVector& Eye, const FVector& Target) const;

    const FAssetEntry* FindAssetEntry(FAssetHandle AssetHandle) const;

    static Uint64 MakeThumbnailKey(FAssetHandle AssetHandle);

private:
    FRenderer* mRenderer{nullptr};

    /** @brief 썸네일과 생성 대기 목록이 참조하는 전역 에셋 서비스 세대입니다. */
    Uint64 mRegistryGeneration{};
    FPreviewScene mPreviewScene{};

    TMap<Uint64, FThumbnail> mThumbnails{};
    TArray<FAssetHandle> mPendingAssetHandles{};
    std::size_t mPendingAssetIndex{};
};
