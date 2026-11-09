/**
 * @file FMeshImportTexture.cpp
 * @brief 원본 재질 텍스처가 없는지, 외부 파일인지, 내장 참조인지 구분하는 값 객체입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportTexture.h"

#include <utility>

namespace Asset {

    /**
     * @brief 전달받은 설정을 보관하도록 FMeshImportTexture 객체를 초기화합니다.
     */
    Asset::FMeshImportTexture::FMeshImportTexture(std::filesystem::path FilePath, Asset::EMeshImportTextureShape Shape)
		: mSource{Asset::EMeshImportTextureSource::ExternalFile},
		  mFilePath{std::move(FilePath)},
		  mShape{Shape} {
    }

    /**
     * @brief 원본 장면 내부 텍스처의 이름 또는 인덱스를 보관하는 참조를 생성합니다.
     */
    Asset::FMeshImportTexture Asset::FMeshImportTexture::FromEmbedded(Core::FString Reference, Core::TArray<Core::Uint8> Data, Asset::EMeshImportTextureShape Shape) {
        Asset::FMeshImportTexture Texture{};
        Texture.mSource = Asset::EMeshImportTextureSource::Embedded;
        Texture.mEmbeddedReference = std::move(Reference);
        Texture.mEmbeddedData = std::move(Data);
        Texture.mShape = Shape;

        return Texture;
    }

    /**
     * @brief 재질 텍스처의 출처 종류를 반환합니다.
     */
    Asset::EMeshImportTextureSource Asset::FMeshImportTexture::GetSource() const {
        return mSource;
    }

    /**
     * @brief 외부 텍스처 파일 경로를 조회합니다.
     */
    const std::filesystem::path& Asset::FMeshImportTexture::GetFilePath() const {
        return mFilePath;
    }

    /**
     * @brief 원본 장면 내부의 텍스처 식별 문자열을 조회합니다.
     */
    const Core::FString& Asset::FMeshImportTexture::GetEmbeddedReference() const {
        return mEmbeddedReference;
    }

    /**
     * @brief 원본에서 추출한 내장 텍스처 바이트를 조회합니다.
     */
    const Core::TArray<Core::Uint8>& Asset::FMeshImportTexture::GetEmbeddedData() const {
        return mEmbeddedData;
    }

    /**
     * @brief 임포터가 판별한 텍스처 형태를 반환합니다.
     */
    Asset::EMeshImportTextureShape Asset::FMeshImportTexture::GetShape() const {
        return mShape;
    }

    /**
     * @brief 원본 장면 정보로 판별한 텍스처 형태를 기록합니다.
     */
    void Asset::FMeshImportTexture::SetShape(Asset::EMeshImportTextureShape Shape) {
        mShape = Shape;
    }

}
