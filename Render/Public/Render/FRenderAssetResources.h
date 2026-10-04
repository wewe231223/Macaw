#pragma once

#include "Render/FMaterialBuffer.h"
#include "Render/FMeshRenderResource.h"
#include "Render/Pipeline/FPipelineRenderResource.h"

class UTexture;
class UFont;

class FRenderAssetResources {
private:
    struct FMeshEntry {
        FMeshRenderResource mResource{};
        Uint64 mRevision{};
    };

    struct FPipelineEntry {
        FPipelineRenderResource mResource{};
        Uint64 mRevision{};
    };

    struct FTextureEntry {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mView{};
        Uint64 mRevision{};
    };

    struct FFontEntry {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> mTexture{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mView{};
        Uint32 mWidth{};
        Uint32 mHeight{};
        Uint64 mRevision{};
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
    TMap<FGuid, FMeshEntry> mMeshes{};
    TMap<FGuid, FPipelineEntry> mPipelines{};
    TMap<FGuid, FTextureEntry> mTextures{};
    TMap<FGuid, FFontEntry> mFonts{};
    FMaterialBuffer mMaterials{};
};
