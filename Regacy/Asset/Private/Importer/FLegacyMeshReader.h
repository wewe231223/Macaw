#pragma once
#include "Asset/FObjInfo.h"

class FLegacyMeshReader {
public:
    static constexpr Uint32 MagicNumber{0x4d45534d}; // MESM : Macaw Engine Static Mesh
    static constexpr Uint32 RawUVVersion{4};

    static bool Load(const FString& FilePath, FGeometry& OutGeoData, Uint32& LoadedVersion);

private:
    static bool ReadFString(std::ifstream& In, FString& OutStr);
};
