#include "pch.h"
#include "Editor/Settings/FEditorConfigManager.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"

bool FEditorConfigManager::Save(const FEditorSettings& Settings, const std::filesystem::path& ConfigPath) {
    rapidjson::Document Document{};

    Document.SetObject();

    FEditorSettings SavedSettings{Settings};
    FArchiveJson Archive{Document, Document.GetAllocator()};

    Archive.SerializeStruct("EditorSettings", SavedSettings);

    return FJsonFile::Save(ConfigPath, Document);
}

bool FEditorConfigManager::Load(FEditorSettings& OutSettings, const std::filesystem::path& ConfigPath) {
    rapidjson::Document Document{};

    if (!FJsonFile::Load(ConfigPath, Document) || !Document.HasMember("EditorSettings") || !Document["EditorSettings"].IsObject()) {
        return false;
    }

    FEditorSettings Settings{OutSettings};
    FArchiveJson Archive{Document["EditorSettings"]};

    Settings.Serialize(Archive);

    if (Archive.HasError()) {
        return false;
    }

    OutSettings = Settings;

    return true;
}
