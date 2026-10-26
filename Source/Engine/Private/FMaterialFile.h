#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

class FAssetManager;
class UMaterial;

class FMaterialFile {
public:
    ReflectTypes(FMaterialFile);

public:
    ReflectStruct(FMaterialFile);
    static bool Load(const std::filesystem::path& Path, FAssetManager& Assets, UMaterial& Material, FString& Error);
    static bool Save(const std::filesystem::path& Path, const FAssetManager& Assets, const UMaterial& Material, FString& Error);

private:
    ReflectMembers();
};
