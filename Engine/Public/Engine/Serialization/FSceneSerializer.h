#pragma once

#include <filesystem>

class UWorld;

class FSceneSerializer final {
public:
    static bool Save(UWorld& World, const std::filesystem::path& ScenePath);
    static bool Load(UWorld& World, const std::filesystem::path& ScenePath);

private:
    static bool LoadInternal(UWorld& World, const std::filesystem::path& ScenePath);
};
