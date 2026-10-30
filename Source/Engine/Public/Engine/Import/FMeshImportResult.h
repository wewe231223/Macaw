#pragma once

#include "Engine/FMeshData.h"
#include "Engine/Import/FMeshImportMaterial.h"

class FMeshImportResult {
public:
    ReflectTypes(FMeshImportResult);

public:
    ReflectStruct(FMeshImportResult);

    FMeshData& GetMeshData();
    const FMeshData& GetMeshData() const;

    TArray<FMeshImportMaterial>& GetMaterials();
    const TArray<FMeshImportMaterial>& GetMaterials() const;

private:
    ReflectMembers();

private:
    FMeshData mMeshData{};
    ReflectProperty(mMeshData, "MeshData", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    TArray<FMeshImportMaterial> mMaterials{};
    ReflectProperty(mMaterials, "Materials", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
