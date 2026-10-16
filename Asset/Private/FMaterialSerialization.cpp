#include "pch.h"
#include "FMaterialSerialization.h"
#include "Serialization/FJsonFile.h"
#include <cstring>

bool RequireMaterialFields(FArchive& Archive, std::initializer_list<std::string_view> Names) {
    if (Archive.IsLoading() && Archive.SupportsNamedFields()) {
        for (std::string_view Name : Names) {
            if (!Archive.HasField(Name)) {
                Archive.SetError();
                return false;
            }
        }
    }

    return !Archive.HasError();
}

bool LoadMaterialDocument(const std::filesystem::path& Path, const char* Kind, rapidjson::Document& Document) {
    if (!FJsonFile::Load(Path, Document) || !Document.HasMember("Kind") || !Document["Kind"].IsString() || Document["Kind"].GetStringLength() != std::strlen(Kind) || std::strcmp(Document["Kind"].GetString(), Kind) != 0 || !Document.HasMember("Version") || !Document["Version"].IsUint() || Document["Version"].GetUint() != 1 || !Document.HasMember("MaterialDataVersion") || !Document["MaterialDataVersion"].IsUint() || Document["MaterialDataVersion"].GetUint() != MaterialDataVersion) {
        return false;
    }

    return true;
}

void InitializeMaterialDocument(const char* Kind, rapidjson::Document& Document) {
    Document.SetObject();
    Document.AddMember("Kind", rapidjson::Value{Kind, Document.GetAllocator()}, Document.GetAllocator());
    Document.AddMember("Version", 1u, Document.GetAllocator());
    Document.AddMember("MaterialDataVersion", MaterialDataVersion, Document.GetAllocator());
}

void SerializeMaterialData(FArchive& Archive, FMaterialGPUData& Data) {
    if (!RequireMaterialFields(Archive, {"BaseColor", "Opacity", "Emissive", "NormalScale", "Metallic", "Roughness", "Specular", "AmbientOcclusion", "OpacityMaskClipValue"})) {
        return;
    }

    Archive.Serialize("BaseColor", Data.mBaseColor);
    Archive.Serialize("Opacity", Data.mOpacity);
    Archive.Serialize("Emissive", Data.mEmissive);
    Archive.Serialize("NormalScale", Data.mNormalScale);
    Archive.Serialize("Metallic", Data.mMetallic);
    Archive.Serialize("Roughness", Data.mRoughness);
    Archive.Serialize("Specular", Data.mSpecular);
    Archive.Serialize("AmbientOcclusion", Data.mAmbientOcclusion);
    Archive.Serialize("OpacityMaskClipValue", Data.mOpacityMaskClipValue);

    if (!IsValidMaterialData(Data)) {
        Archive.SetError();
    }
}
