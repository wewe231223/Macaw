#include "pch.h"
#include "Importer/FLegacyMeshReader.h"
#include <fstream>

bool FLegacyMeshReader::Load(const FString& FilePath, FGeometry& OutGeoData, Uint32& LoadedVersion) {
    LoadedVersion = {};

    std::ifstream In{FilePath.c_str(), std::ios::binary};

    if (!In.is_open()) {
        return false;
    }

    Uint32 Magic{0};
    Uint32 Version{0};

    In.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
    In.read(reinterpret_cast<char*>(&Version), sizeof(Version));

    //버전이 안 맞으면 바이너리를 읽어오지 않는다.
    if (Magic != MagicNumber || (Version != 3 && Version != RawUVVersion)) {
        return false;
    }

    LoadedVersion = Version;

    Uint32 PositionCount{0};

    In.read(reinterpret_cast<char*>(&PositionCount), sizeof(PositionCount));
    OutGeoData.mPositions.resize(PositionCount);
    In.read(reinterpret_cast<char*>(OutGeoData.mPositions.data()), sizeof(FVector) * PositionCount);

    Uint32 NormalCount{0};

    In.read(reinterpret_cast<char*>(&NormalCount), sizeof(NormalCount));
    OutGeoData.mNormals.resize(NormalCount);
    In.read(reinterpret_cast<char*>(OutGeoData.mNormals.data()), sizeof(FVector) * NormalCount);

    Uint32 UVCount{0};

    In.read(reinterpret_cast<char*>(&UVCount), sizeof(UVCount));
    OutGeoData.mTexCoords.resize(UVCount);
    In.read(reinterpret_cast<char*>(OutGeoData.mTexCoords.data()), sizeof(FVector2) * UVCount);

    Uint32 IndexCount{0};

    In.read(reinterpret_cast<char*>(&IndexCount), sizeof(IndexCount));
    OutGeoData.mIndices.resize(IndexCount);
    In.read(reinterpret_cast<char*>(OutGeoData.mIndices.data()), sizeof(Uint32) * IndexCount);

    //Material File Name
    ReadFString(In, OutGeoData.mMaterialFileName);

    //Material Names
    Uint32 MaterialNamesCount{0};

    In.read(reinterpret_cast<char*>(&MaterialNamesCount), sizeof(MaterialNamesCount));
    OutGeoData.mMaterialNames.resize(MaterialNamesCount);

    for (Uint32 I{0}; I < MaterialNamesCount; I++) {
        FString MaterialName{};

        if (!ReadFString(In, MaterialName)) {
            return false;
        }

        OutGeoData.mMaterialNames[I] = FName{MaterialName};
    }

    //SubMeshIndexCounts
    Uint32 SubMeshIndexCount{0};

    In.read(reinterpret_cast<char*>(&SubMeshIndexCount), sizeof(SubMeshIndexCount));
    OutGeoData.mSubMeshIndexCounts.resize(SubMeshIndexCount);
    In.read(reinterpret_cast<char*>(OutGeoData.mSubMeshIndexCounts.data()), sizeof(Uint32) * SubMeshIndexCount);

    Uint32 ColorCount{0};

    In.read(reinterpret_cast<char*>(&ColorCount), sizeof(ColorCount));
    OutGeoData.mColors.resize(ColorCount);
    In.read(reinterpret_cast<char*>(OutGeoData.mColors.data()), sizeof(FColor4) * ColorCount);

    return static_cast<bool>(In);
}

bool FLegacyMeshReader::ReadFString(std::ifstream& In, FString& OutStr) {
    Uint32 Len{0};

    In.read(reinterpret_cast<char*>(&Len), sizeof(Len));

    if (!In)
        return false;

    OutStr.resize(Len);
    In.read(OutStr.data(), Len);

    return static_cast<bool>(In);
}
