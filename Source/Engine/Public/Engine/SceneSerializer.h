#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <filesystem>

class FArchive;
class FWorldContext;

class FSceneSerializer {
public:
    ReflectTypes(FSceneSerializer);

public:
    ReflectStruct(FSceneSerializer);

    static bool Save(const std::filesystem::path& Path, FWorldContext& Context, FString& Error);
    static std::unique_ptr<FWorldContext> Load(const std::filesystem::path& Path, FString& Error);
    static void Serialize(FArchive& Archive, FWorldContext& Context);

private:
    ReflectMembers();
};
