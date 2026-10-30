/**
 * @file FMeshImportTexture.cpp
 * @brief 원본 재질 텍스처가 없는지, 외부 파일인지, 내장 참조인지 구분하는 값 객체입니다.
 * @ingroup Engine
 */

#include "Engine/Import/FMeshImportTexture.h"

#include <utility>

namespace Engine {

    Engine::FMeshImportTexture::FMeshImportTexture(std::filesystem::path FilePath)
		: mSource{Engine::EMeshImportTextureSource::ExternalFile},
		  mFilePath{std::move(FilePath)} {
    }

    Engine::FMeshImportTexture Engine::FMeshImportTexture::FromEmbedded(Core::FString Reference) {
        Engine::FMeshImportTexture Texture{};
        Texture.mSource = Engine::EMeshImportTextureSource::Embedded;
        Texture.mEmbeddedReference = std::move(Reference);
        return Texture;
    }

    Engine::EMeshImportTextureSource Engine::FMeshImportTexture::GetSource() const {
        return mSource;
    }

    const std::filesystem::path& Engine::FMeshImportTexture::GetFilePath() const {
        return mFilePath;
    }

    const Core::FString& Engine::FMeshImportTexture::GetEmbeddedReference() const {
        return mEmbeddedReference;
    }

}
