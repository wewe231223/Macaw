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
    bool mSky{};

    Uint32 mFirstIndex{};
    Uint32 mIndexCount{};
    Uint32 mLODLevel{};
    Uint32 mOriginalIndexCount{};
    Uint32 mMaterialSlotIndex{};
};

void BuildMeshBatchElements(const UMesh& Mesh, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements);
FAssetHandle ResolveMeshMaterial(const UMesh& Mesh, const TArray<FMeshMaterialBinding>& Overrides, Uint32 SlotIndex);
FAssetHandle SelectMaterialPipeline(const UMaterial& Material, bool Overlay = false);
bool BuildMeshDrawState(const FMeshBatch& Mesh, const FMeshBatchElement& Element, const UMaterial& Material, const FMaterialBuffer& Materials, FMeshDrawState& OutState, Uint32& OutMaterialIndex, bool Overlay = false);
