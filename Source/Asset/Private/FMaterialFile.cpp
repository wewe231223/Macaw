/**
 * @file FMaterialFile.cpp
 * @brief 재질 직렬화와 프로젝트 참조 검증을 구현합니다.
 * @ingroup Asset
 */

#include "FMaterialFile.h"
#include "FMaterialAssetData.h"
#include "Asset/FAssetManager.h"
#include "Asset/UMaterial.h"
#include "Asset/FAssetFile.h"
#include "Serialization/MemoryArchive.h"

namespace Asset {

    /**
     * @brief 단일 머티리얼 설정과 텍스처 참조를 읽고 검증합니다.
     */
    bool Asset::FMaterialFile::Load(const std::filesystem::path& Path, Asset::FAssetManager& Assets, Asset::UMaterial& Material, Core::FString& Error) {
        Core::TArray<Core::Uint8> Payload{};

        if (!Asset::FAssetFile::Load(Path, Asset::EAssetType::Material, Payload, Error)) {
            return false;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Asset::FMaterialAssetData Data{};
        Data.Serialize(Archive);

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
            Error = Archive.HasError() ? Archive.GetError() : "Unexpected material asset data";

            return false;
        }

        return Data.ValidateReferences(Assets, Error) && Data.ApplyTo(Material, Error) && Material.ResolveTexture(Assets, Error);
    }

    /**
     * @brief 설정을 기록한 임시 파일로 기존 재질 에셋 파일을 교체합니다.
     */
    bool Asset::FMaterialFile::Save(const std::filesystem::path& Path, const Asset::FAssetManager& Assets, const Asset::UMaterial& Material, Core::FString& Error) {
        Error.clear();

        Asset::FMaterialAssetData Data{Material};

        if (!Data.ValidateReferences(Assets, Error)) {
            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};

        Data.Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        return Asset::FAssetFile::Save(Path, Asset::EAssetType::Material, Payload, Error);
    }

}
