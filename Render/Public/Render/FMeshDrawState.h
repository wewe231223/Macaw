#pragma once
#include "Render/FMaterialBuffer.h"
#include "RenderCore/FMaterialChunkSignature.h"
#include "RenderCore/FMaterialBlendMode.h"
#include "RenderCore/FMeshBatch.h"

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

bool BuildMeshDrawState(const FMeshBatch& Mesh, const FMeshBatchElement& Element, const IAssetRegistry& Registry, const FMaterialBuffer& Materials, FMeshDrawState& OutState, Uint32& OutMaterialIndex);
