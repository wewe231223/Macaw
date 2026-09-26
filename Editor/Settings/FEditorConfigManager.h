#pragma once
#include <filesystem>
#include "Core/Channel/FEditorSettings.h"
#include "Core/Asset/IAssetRegistry.h"

class FEditorConfigManager {
public:
    static bool Save(FEditorSettings& Settings, const IAssetRegistry* AssetRegistry = nullptr, const std::filesystem::path& ConfigPath = "Editor.ini");
    static bool Load(FEditorSettings& OutSettings, const IAssetRegistry* AssetRegistry = nullptr, const std::filesystem::path& ConfigPath = "Editor.ini");

private:
    FEditorConfigManager() = default;
    ~FEditorConfigManager() = default;
};
