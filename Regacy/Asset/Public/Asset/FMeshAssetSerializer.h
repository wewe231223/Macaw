#pragma once

#include "Asset/FMeshData.h"
#include <filesystem>

class FMeshAssetSerializer {
public:
    static bool Save(const std::filesystem::path& Path, const FMeshData& Data);
    static bool Load(const std::filesystem::path& Path, FMeshData& Data);
};
