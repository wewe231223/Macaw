#pragma once

#include <filesystem>

class USurfaceOpaque;

class FMaterialAssetSerializer {
public:
    static bool Save(const std::filesystem::path& Path, const USurfaceOpaque& Material);
    static bool Load(const std::filesystem::path& Path, USurfaceOpaque& Material);
};
