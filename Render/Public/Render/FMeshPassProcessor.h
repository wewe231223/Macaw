#pragma once
#include "Render/FMeshDrawCommandCache.h"
#include "RenderCore/FMeshBatch.h"

class FRenderAssetResources;

class FMeshPassProcessor {
public:
    FMeshPassProcessor(ERenderPass Pass, const IAssetRegistry& Registry, const FMaterialBuffer& Materials, FRenderAssetResources* Resources, FMeshDrawCommandCache& Cache);
    ~FMeshPassProcessor();

public:
    void AddMeshBatch(const FMeshBatch& Mesh, TArray<Uint32>& OutCommands);

private:
    bool AcceptsMaterial(EMaterialBlendMode BlendMode) const;
    bool BuildShaderBindings(const FMeshBatch& Mesh, FMeshDrawCommand& Command);

private:
    ERenderPass mPass{ERenderPass::Opaque};
    const IAssetRegistry& mRegistry;
    const FMaterialBuffer& mMaterials;
    FRenderAssetResources* mResources{nullptr};
    FMeshDrawCommandCache& mCache;
};
