/**
 * @file FRenderAssetResources.h
 * @brief 장치별 GPU 에셋 캐시와 전역 에셋 생존 여부 검사를 정의합니다.
 */

#pragma once

#include "Render/FMaterialBuffer.h"
#include "Render/FMeshRenderResource.h"
#include "Render/Pipeline/FPipelineRenderResource.h"
#include "Render/FRenderAssetStamp.h"
#include "Core/Base/TCachedValue.h"

class UTexture;
class UFont;

class FRenderAssetResources {
private:
    struct FFontResource {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> mTexture{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mView{};
        Uint32 mWidth{};
        Uint32 mHeight{};
    };

    struct FFontAtlasStamp {
        FRenderAssetStamp mAsset{};
        Uint32 mWidth{};
        Uint32 mHeight{};

        bool operator==(const FFontAtlasStamp&) const = default;
    };

public:
    FRenderAssetResources() = default;
    ~FRenderAssetResources() = default;

    FRenderAssetResources(const FRenderAssetResources&) = delete;

    FRenderAssetResources& operator=(const FRenderAssetResources&) = delete;

    FRenderAssetResources(FRenderAssetResources&&) = delete;

    FRenderAssetResources& operator=(FRenderAssetResources&&) = delete;

public:
    bool Initialize(ID3D11Device* Device, bool UseLOD = true);

    /**
     * @brief 전역 레지스트리에 존재하지 않는 에셋의 GPU 캐시를 해제합니다.
     * @note 활성 에셋 서비스가 없으면 모든 에셋 캐시를 비웁니다. 장치와 머티리얼 버퍼 자체는 유지합니다.
     */
    void Prune();
    void Reset();

    const FMeshRenderResource* GetMesh(const UMesh& Mesh);
    const FPipelineRenderResource* GetPipeline(const UPipeline& Pipeline);
    ID3D11ShaderResourceView* GetTexture(const UTexture& Texture);
    ID3D11ShaderResourceView* GetFontAtlas(const UFont& Font, ID3D11DeviceContext* Context);
    FMaterialBuffer& GetMaterialBuffer();

private:
    ID3D11Device* mDevice{nullptr};
    bool mUseLOD{true};
    TMap<FGuid, TCachedValue<FMeshRenderResource, FRenderAssetStamp>> mMeshes{};
    TMap<FGuid, TCachedValue<FPipelineRenderResource, FRenderAssetStamp>> mPipelines{};
    TMap<FGuid, TCachedValue<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, FRenderAssetStamp>> mTextures{};
    TMap<FGuid, TCachedValue<FFontResource, FFontAtlasStamp>> mFonts{};
    FMaterialBuffer mMaterials{};
};
