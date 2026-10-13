#include "pch.h"
#include "Render/FStaticMeshBatchCollector.h"
#include "Render/FMeshDrawState.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMesh.h"
#include "Asset/FLODSettings.h"

#include <algorithm>

FStaticMeshBatchCollector::FStaticMeshBatchCollector(const IAssetRegistry& Registry, TArray<FMeshBatch>& Meshes)
	: mRegistry(Registry),
	  mMeshes(Meshes) {
}

Uint32 FStaticMeshBatchCollector::GetMeshLODCount(FAssetHandle MeshHandle) const {
    const UMesh* Mesh{mRegistry.ResolveAsset<UMesh>(MeshHandle)};

    return Mesh != nullptr ? std::min(Mesh->GetLODCount(), GLODCount) : 0;
}

void FStaticMeshBatchCollector::GetMeshElements(FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) const {
    const UMesh* Mesh{mRegistry.ResolveAsset<UMesh>(MeshHandle)};

    if (Mesh != nullptr) {
        BuildMeshBatchElements(*Mesh, LODLevel, OutElements);
    } else {
        OutElements.clear();
    }
}

void FStaticMeshBatchCollector::DrawMesh(const FMeshBatch& Mesh) {
    mMeshes.push_back(Mesh);
}
