/**
 * @file FMeshImportTexture.cpp
 * @brief 원본 재질 텍스처가 없는지, 외부 파일인지, 내장 참조인지 구분하는 값 객체입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportTexture.h"

#include <utility>

namespace Asset {

    Asset::FMeshImportTexture::FMeshImportTexture(std::filesystem::path FilePath, Asset::EMeshImportTextureShape Shape)
		: mSource{Asset::EMeshImportTextureSource::ExternalFile},
		  mFilePath{std::move(FilePath)},
		  mShape{Shape} {
    }

    Asset::FMeshImportTexture Asset::FMeshImportTexture::FromEmbedded(Core::FString Reference, Core::TArray<Core::Uint8> Data, Asset::EMeshImportTextureShape Shape) {
        Asset::FMeshImportTexture Texture{};
        Texture.mSource = Asset::EMeshImportTextureSource::Embedded;
        Texture.mEmbeddedReference = std::move(Reference);
        Texture.mEmbeddedData = std::move(Data);
        Texture.mShape = Shape;
        return Texture;
    }

    Asset::EMeshImportTextureSource Asset::FMeshImportTexture::GetSource() const {
        return mSource;
    }

    const std::filesystem::path& Asset::FMeshImportTexture::GetFilePath() const {
        return mFilePath;
    }

    const Core::FString& Asset::FMeshImportTexture::GetEmbeddedReference() const {
        return mEmbeddedReference;
    }

    const Core::TArray<Core::Uint8>& Asset::FMeshImportTexture::GetEmbeddedData() const {
        return mEmbeddedData;
    }

    Asset::EMeshImportTextureShape Asset::FMeshImportTexture::GetShape() const {
        return mShape;
    }

    void Asset::FMeshImportTexture::SetShape(Asset::EMeshImportTextureShape Shape) {
        mShape = Shape;
    }

}
