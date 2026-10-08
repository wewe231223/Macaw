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
    bool Initialize(ID3D11Device* Device);
    void Prune(const IAssetRegistry& Registry);
    void Reset();

    const FMeshRenderResource* GetMesh(const UMesh& Mesh);
    const FPipelineRenderResource* GetPipeline(const UPipeline& Pipeline);
    ID3D11ShaderResourceView* GetTexture(const UTexture& Texture);
    ID3D11ShaderResourceView* GetFontAtlas(const UFont& Font, ID3D11DeviceContext* Context);
    FMaterialBuffer& GetMaterialBuffer();

private:
    ID3D11Device* mDevice{nullptr};
    TMap<FGuid, TCachedValue<FMeshRenderResource, FRenderAssetStamp>> mMeshes{};
    TMap<FGuid, TCachedValue<FPipelineRenderResource, FRenderAssetStamp>> mPipelines{};
    TMap<FGuid, TCachedValue<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, FRenderAssetStamp>> mTextures{};
    TMap<FGuid, TCachedValue<FFontResource, FFontAtlasStamp>> mFonts{};
    FMaterialBuffer mMaterials{};
};
