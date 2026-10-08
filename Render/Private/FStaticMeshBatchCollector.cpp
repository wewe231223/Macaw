#include "pch.h"
#include "Render/FStaticMeshBatchCollector.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMesh.h"
#include "Asset/FLODSettings.h"

#include <algorithm>

FStaticMeshBatchCollector::FStaticMeshBatchCollector(const IAssetRegistry& Registry, Uint32 ObjectIndex, TArray<FStaticMeshBatch>& Meshes)
	: mRegistry(Registry),
	  mObjectIndex(ObjectIndex),
	  mMeshes(Meshes) {
}

Uint32 FStaticMeshBatchCollector::GetMeshLODCount(FAssetHandle MeshHandle) const {
    const UMesh* Mesh{mRegistry.ResolveAsset<UMesh>(MeshHandle)};

    return Mesh != nullptr ? std::min(Mesh->GetLODCount(), GLODCount) : 0;
}

void FStaticMeshBatchCollector::GetMeshElements(FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) const {
    OutElements.clear();

    const UMesh* Mesh{mRegistry.ResolveAsset<UMesh>(MeshHandle)};

    if (Mesh == nullptr || !Mesh->HasLOD(static_cast<int>(LODLevel))) {
        return;
    }

    const TArray<UMesh::FSubMesh>& Sections{Mesh->GetSubMeshes(static_cast<int>(LODLevel))};

    if (Sections.empty()) {
        const Uint32 Count{Mesh->GetIndexCount(static_cast<int>(LODLevel))};

        if (Count != 0) {
            OutElements.push_back(FMeshBatchElement{0, Count, 0, Mesh->GetIndexCount(0)});
        }

        return;
    }

    for (const UMesh::FSubMesh& Section : Sections) {
        if (Section.mIndexCount != 0) {
            OutElements.push_back(FMeshBatchElement{Section.mFirstIndex, Section.mIndexCount, Section.mMaterialGroupIndex, Section.mSourceIndexCount != 0 ? Section.mSourceIndexCount : Section.mIndexCount});
        }
    }
}

void FStaticMeshBatchCollector::DrawMesh(const FMeshBatch& Mesh) {
    mMeshes.push_back(FStaticMeshBatch{Mesh, mObjectIndex});
}
