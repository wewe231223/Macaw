#include "pch.h"
#include "Editor/Settings/FEditorConfigManager.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"

FEditorConfigManager& FEditorConfigManager::Get() {
    static FEditorConfigManager Instance{};

    return Instance;
}

FEditorSettings& FEditorConfigManager::GetSettings() {
    return mSettings;
}

const FEditorSettings& FEditorConfigManager::GetSettings() const {
    return mSettings;
}

bool FEditorConfigManager::Reload(const std::filesystem::path& ConfigPath) {
    rapidjson::Document Document{};

    if (!FJsonFile::Load(ConfigPath, Document) || !Document.HasMember("EditorSettings") || !Document["EditorSettings"].IsObject()) {
        return false;
    }

    FEditorSettings Settings{mSettings};
    FArchiveJson Archive{Document["EditorSettings"]};

    Settings.Serialize(Archive);

    if (Archive.HasError()) {
        return false;
    }

    mSettings = Settings;

    return true;
}

bool FEditorConfigManager::Flush(const std::filesystem::path& ConfigPath) const {
    rapidjson::Document Document{};

    Document.SetObject();

    FEditorSettings SavedSettings{mSettings};
    FArchiveJson Archive{Document, Document.GetAllocator()};

    Archive.SerializeStruct("EditorSettings", SavedSettings);

    return FJsonFile::Save(ConfigPath, Document);
}
