#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

class FMeshImportResult;

class IMeshImporter {
public:
    ReflectTypes(IMeshImporter);

public:
    virtual ~IMeshImporter();

public:
    ReflectStruct(IMeshImporter);

    static const IMeshImporter& GetDefault();

    virtual bool Import(const std::filesystem::path& Source, FMeshImportResult& Result, FString& Error) const = 0;

private:
    ReflectMembers();
};
