#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Render/FStaticMeshBatchCollector.h"
#include "Render/FMeshDrawState.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMesh.h"
#include "Asset/FLODSettings.h"

#include <algorithm>

FStaticMeshBatchCollector::FStaticMeshBatchCollector(TArray<FMeshBatch>& Meshes)
	: mMeshes{Meshes} {
}

Uint32 FStaticMeshBatchCollector::GetMeshLODCount(FAssetHandle MeshHandle) const {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};
    const UMesh* Mesh{Registry != nullptr ? Registry->ResolveAsset<UMesh>(MeshHandle) : nullptr};

    return Mesh != nullptr ? std::min(Mesh->GetLODCount(), GLODCount) : 0;
}

void FStaticMeshBatchCollector::GetMeshElements(FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) const {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};
    const UMesh* Mesh{Registry != nullptr ? Registry->ResolveAsset<UMesh>(MeshHandle) : nullptr};

    if (Mesh != nullptr) {
        BuildMeshBatchElements(*Mesh, LODLevel, OutElements);
    } else {
        OutElements.clear();
    }
}

void FStaticMeshBatchCollector::DrawMesh(const FMeshBatch& Mesh) {
    mMeshes.push_back(Mesh);
}
