#include "pch.h"
#include "Render/FMeshDrawState.h"
#include "Asset/UMaterial.h"

bool BuildMeshDrawState(const FMeshBatch& Mesh, const FMeshBatchElement& Element, const IAssetRegistry& Registry, const FMaterialBuffer& Materials, FMeshDrawState& OutState, Uint32& OutMaterialIndex) {
    OutState = {};
    OutMaterialIndex = UINT32_MAX;

    const UMaterial* Material{Registry.ResolveAsset<UMaterial>(Mesh.mMaterialHandle)};

    if (Material == nullptr || Element.mIndexCount == 0) {
        return false;
    }

    const Uint32 Group{Materials.GetMaterialIndex(*Material, Element.mMaterialGroupIndex) != UINT32_MAX ? Element.mMaterialGroupIndex : 0};

    OutMaterialIndex = Materials.GetMaterialIndex(*Material, Group);

    if (OutMaterialIndex == UINT32_MAX) {
        return false;
    }

    OutState.mPipelineHandle = Mesh.mPipelineHandle;
    OutState.mMeshHandle = Mesh.mMeshHandle;
    OutState.mTextureSignature = Material->BuildChunkSignature(Group);
    OutState.mBlendMode = Material->GetBlendMode(Group);
    OutState.mFirstIndex = Element.mFirstIndex;
    OutState.mIndexCount = Element.mIndexCount;
    OutState.mLODLevel = Mesh.mLODLevel;
    OutState.mOriginalIndexCount = Element.mOriginalIndexCount;

    return true;
}
