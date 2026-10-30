#include "Engine/Import/FMeshImportResult.h"

FMeshData& FMeshImportResult::GetMeshData() {
    return mMeshData;
}

const FMeshData& FMeshImportResult::GetMeshData() const {
    return mMeshData;
}

TArray<FMeshImportMaterial>& FMeshImportResult::GetMaterials() {
    return mMaterials;
}

const TArray<FMeshImportMaterial>& FMeshImportResult::GetMaterials() const {
    return mMaterials;
}
