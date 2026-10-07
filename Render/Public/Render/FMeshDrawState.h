#pragma once
#include "Render/FMaterialBuffer.h"
#include "RenderCore/FMaterialChunkSignature.h"
#include "RenderCore/FMaterialBlendMode.h"

class UMaterial;
class UMesh;

struct FMeshDrawState {
    FAssetHandle mPipelineHandle{};
    FAssetHandle mMeshHandle{};
    FMaterialChunkSignature mTextureSignature{};
    EMaterialBlendMode mBlendMode{EMaterialBlendMode::Opaque};

    Uint32 mFirstIndex{};
    Uint32 mIndexCount{};
    Uint32 mLODLevel{};
    Uint32 mOriginalIndexCount{};
};

struct FRenderBatchTemplate {
    FMeshDrawState mState{};

    Uint32 mMaterialIndex{};
};

void AppendMeshDrawTemplates(const UMesh& Mesh, const UMaterial& Material, const FMaterialBuffer& Materials, FAssetHandle PipelineHandle, FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FRenderBatchTemplate>& OutTemplates);
