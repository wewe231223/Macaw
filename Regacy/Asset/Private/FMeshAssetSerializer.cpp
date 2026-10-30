#include "pch.h"
#include "Asset/FMeshAssetSerializer.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "Serialization/FArchiveMemory.h"
#include <fstream>
#include <windows.h>

namespace {
    constexpr Uint32 Magic{0x3548534D};
    constexpr Uint32 Version{1};

    void SerializeData(FArchive& Archive, FMeshData& Data) {
        SerializeNativeValue(Archive, "Positions", Data.mPositions);
        SerializeNativeValue(Archive, "Normals", Data.mNormals);
        SerializeNativeValue(Archive, "UVs", Data.mUVs);
        SerializeNativeValue(Archive, "Colors", Data.mColors);
        SerializeNativeValue(Archive, "Indices", Data.mIndices);

        std::size_t SectionCount{Data.mSections.size()};

        Archive.BeginArrayScope("Sections", SectionCount);

        if (Archive.IsLoading()) {
            Data.mSections.resize(SectionCount);
        }

        for (FMeshSection& Section : Data.mSections) {
            Archive.Serialize("FirstIndex", Section.mFirstIndex);
            Archive.Serialize("IndexCount", Section.mIndexCount);
            Archive.Serialize("MaterialSlotIndex", Section.mMaterialSlotIndex);
        }

        Archive.EndArrayScope();
        SerializeNativeValue(Archive, "MaterialSlots", Data.mMaterialSlots);
    }
}

bool FMeshAssetSerializer::Save(const std::filesystem::path& Path, const FMeshData& Data) {
    TArray<Uint8> Bytes{};
    FArchiveMemory Archive{Bytes};
    FMeshData Copy{Data};

    SerializeData(Archive, Copy);

    if (Archive.HasError()) {
        return false;
    }

    const std::filesystem::path Temporary{Path.string() + "." + FGuid::NewGuid().ToString().c_str() + ".tmp"};
    std::ofstream Output{Temporary, std::ios::binary | std::ios::trunc};

    Output.write(reinterpret_cast<const char*>(&Magic), sizeof(Magic));
    Output.write(reinterpret_cast<const char*>(&Version), sizeof(Version));
    Output.write(reinterpret_cast<const char*>(Bytes.data()), static_cast<std::streamsize>(Bytes.size()));

    const bool Written{static_cast<bool>(Output)};

    Output.close();

    if (Written && Output && MoveFileExW(Temporary.c_str(), Path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return true;
    }

    std::error_code Error{};

    std::filesystem::remove(Temporary, Error);

    return false;
}

bool FMeshAssetSerializer::Load(const std::filesystem::path& Path, FMeshData& Data) {
    std::ifstream Input{Path, std::ios::binary | std::ios::ate};

    if (!Input || Input.tellg() < 8) {
        return false;
    }

    const std::size_t Size{static_cast<std::size_t>(Input.tellg()) - 8};
    Uint32 StoredMagic{};
    Uint32 StoredVersion{};

    Input.seekg(0);
    Input.read(reinterpret_cast<char*>(&StoredMagic), sizeof(StoredMagic));
    Input.read(reinterpret_cast<char*>(&StoredVersion), sizeof(StoredVersion));

    if (StoredMagic != Magic || StoredVersion != Version) {
        return false;
    }

    TArray<Uint8> Bytes{};

    Bytes.resize(Size);

    Input.read(reinterpret_cast<char*>(Bytes.data()), static_cast<std::streamsize>(Size));

    if (!Input) {
        return false;
    }

    const TArray<Uint8>& Source{Bytes};
    FArchiveMemory Archive{Source};
    FMeshData Loaded{};

    SerializeData(Archive, Loaded);

    if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
        return false;
    }

    Data = std::move(Loaded);

    return true;
}
