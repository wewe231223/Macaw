#include "Engine/Import/FMeshImportTexture.h"

#include <utility>

FMeshImportTexture::FMeshImportTexture(std::filesystem::path FilePath)
	: mSource{EMeshImportTextureSource::ExternalFile},
	  mFilePath{std::move(FilePath)} {
}

FMeshImportTexture FMeshImportTexture::FromEmbedded(FString Reference) {
    FMeshImportTexture Texture{};
    Texture.mSource = EMeshImportTextureSource::Embedded;
    Texture.mEmbeddedReference = std::move(Reference);
    return Texture;
}

EMeshImportTextureSource FMeshImportTexture::GetSource() const {
    return mSource;
}

const std::filesystem::path& FMeshImportTexture::GetFilePath() const {
    return mFilePath;
}

const FString& FMeshImportTexture::GetEmbeddedReference() const {
    return mEmbeddedReference;
}
