#include "pch.h"
#include "Render/FMeshDrawState.h"
#include "Asset/UMaterial.h"
#include "Asset/UMesh.h"

void BuildMeshBatchElements(const UMesh& Mesh, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) {
    OutElements.clear();

    if (!Mesh.HasLOD(static_cast<int>(LODLevel))) {
        return;
    }

    const TArray<UMesh::FSubMesh>& Sections{Mesh.GetSubMeshes(static_cast<int>(LODLevel))};

    if (Sections.empty()) {
        const Uint32 Count{Mesh.GetIndexCount(static_cast<int>(LODLevel))};

        if (Count != 0) {
            OutElements.push_back(FMeshBatchElement{0, Count, 0, Mesh.GetIndexCount(0)});
        }

        return;
    }

    for (const UMesh::FSubMesh& Section : Sections) {
        if (Section.mIndexCount != 0) {
            OutElements.push_back(FMeshBatchElement{Section.mFirstIndex, Section.mIndexCount, Section.mMaterialGroupIndex, Section.mSourceIndexCount != 0 ? Section.mSourceIndexCount : Section.mIndexCount});
        }
    }
}

bool BuildMeshDrawState(const FMeshBatch& Mesh, const FMeshBatchElement& Element, const UMaterial& Material, const FMaterialBuffer& Materials, FMeshDrawState& OutState, Uint32& OutMaterialIndex) {
    OutState = {};
    OutMaterialIndex = UINT32_MAX;

    if (Element.mIndexCount == 0) {
        return false;
    }

    const Uint32 RequestedMaterialIndex{Materials.GetMaterialIndex(Material, Element.mMaterialGroupIndex)};
    const Uint32 Group{RequestedMaterialIndex != UINT32_MAX ? Element.mMaterialGroupIndex : 0};

    OutMaterialIndex = RequestedMaterialIndex != UINT32_MAX ? RequestedMaterialIndex : Materials.GetMaterialIndex(Material, Group);

    if (OutMaterialIndex == UINT32_MAX) {
        return false;
    }

    OutState.mPipelineHandle = Mesh.mPipelineHandle;
    OutState.mMeshHandle = Mesh.mMeshHandle;
    OutState.mTextureSignature = Material.BuildChunkSignature(Group);
    OutState.mBlendMode = Material.GetBlendMode(Group);
    OutState.mFirstIndex = Element.mFirstIndex;
    OutState.mIndexCount = Element.mIndexCount;
    OutState.mLODLevel = Mesh.mLODLevel;
    OutState.mOriginalIndexCount = Element.mOriginalIndexCount;
    OutState.mMaterialGroupIndex = Group;

    return true;
}
