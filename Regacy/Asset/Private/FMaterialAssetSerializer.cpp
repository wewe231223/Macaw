#include "pch.h"
#include "Asset/FMaterialAssetSerializer.h"
#include "Asset/USurfaceOpaque.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"

bool FMaterialAssetSerializer::Save(const std::filesystem::path& Path, const USurfaceOpaque& Material) {
    rapidjson::Document Document{};

    Document.SetObject();

    FArchiveJson Archive{Document, Document.GetAllocator()};
    FSurfaceParameters Parameters{Material.GetParameters()};
    EMaterialShader Shader{Material.GetShader()};
    Uint32 Version{1};

    Archive.Serialize("Version", Version);
    SerializeNativeValue(Archive, "Shader", Shader);
    SerializeNativeValue(Archive, "Parameters", Parameters);

    return !Archive.HasError() && FJsonFile::Save(Path, Document);
}

bool FMaterialAssetSerializer::Load(const std::filesystem::path& Path, USurfaceOpaque& Material) {
    rapidjson::Document Document{};

    if (!FJsonFile::Load(Path, Document) || !Document.IsObject()) {
        return false;
    }

    FArchiveJson Archive{Document};
    FSurfaceParameters Parameters{};
    EMaterialShader Shader{};
    Uint32 Version{};

    Archive.Serialize("Version", Version);
    SerializeNativeValue(Archive, "Shader", Shader);
    SerializeNativeValue(Archive, "Parameters", Parameters);

    if (Archive.HasError() || Version != 1 || Shader > EMaterialShader::Gizmo || !Material.SetParameters(Parameters)) {
        return false;
    }

    Material.SetShader(Shader);

    return true;
}
