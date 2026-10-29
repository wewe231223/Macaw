#pragma once

#include "Core/ReflectionMacros.h"

#include <assimp/DefaultIOSystem.h>

class FMeshImportIOSystem final : public Assimp::IOSystem {
public:
    ReflectTypes(FMeshImportIOSystem);

public:
    ReflectStruct(FMeshImportIOSystem);

    bool Exists(const char* File) const override;
    char getOsSeparator() const override;
    Assimp::IOStream* Open(const char* File, const char* Mode = "rb") override;
    void Close(Assimp::IOStream* Stream) override;
    bool ComparePaths(const char* First, const char* Second) const override;

private:
    ReflectMembers();

private:
    Assimp::DefaultIOSystem mDefaultIOSystem{};
    ReflectProperty(mDefaultIOSystem, "DefaultIOSystem", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
