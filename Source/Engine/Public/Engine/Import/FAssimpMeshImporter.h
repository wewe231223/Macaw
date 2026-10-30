#pragma once

#include "Engine/Import/IMeshImporter.h"

class FAssimpMeshImporter final : public IMeshImporter {
public:
    ReflectTypes(FAssimpMeshImporter, IMeshImporter);

public:
    ReflectStruct(FAssimpMeshImporter);

    bool Import(const std::filesystem::path& Source, FMeshImportResult& Result, FString& Error) const override;

private:
    ReflectMembers();
};
