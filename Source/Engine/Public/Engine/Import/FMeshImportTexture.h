#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

#include <filesystem>

enum class EMeshImportTextureSource : Uint8 {
    None,
    ExternalFile,
    Embedded
};

ReflectEnum(EMeshImportTextureSource, None, ExternalFile, Embedded);

class FMeshImportTexture {
public:
    ReflectTypes(FMeshImportTexture);

public:
    FMeshImportTexture() = default;

    explicit FMeshImportTexture(std::filesystem::path FilePath);

public:
    ReflectStruct(FMeshImportTexture);

    static FMeshImportTexture FromEmbedded(FString Reference);

    EMeshImportTextureSource GetSource() const;
    const std::filesystem::path& GetFilePath() const;
    const FString& GetEmbeddedReference() const;

private:
    ReflectMembers();

private:
    EMeshImportTextureSource mSource{EMeshImportTextureSource::None};
    ReflectProperty(mSource, "Source", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::filesystem::path mFilePath{};
    ReflectProperty(mFilePath, "FilePath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FString mEmbeddedReference{};
    ReflectProperty(mEmbeddedReference, "EmbeddedReference", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
