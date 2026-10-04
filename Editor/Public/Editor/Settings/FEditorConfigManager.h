#pragma once

#include <filesystem>
#include "Editor/Settings/FEditorSettings.h"

class FEditorConfigManager final {
public:
    static bool Save(const FEditorSettings& Settings, const std::filesystem::path& ConfigPath = "Editor.ini");
    static bool Load(FEditorSettings& OutSettings, const std::filesystem::path& ConfigPath = "Editor.ini");
};
