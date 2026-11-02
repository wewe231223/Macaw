/**
 * @file FMeshImportResult.cpp
 * @brief 임포트한 메시 목록과 원본 재질·스켈레톤 본 목록을 함께 보관하는 메시 임포트 결과입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportResult.h"

namespace Asset {

    Core::TArray<Asset::FMeshImportMesh>& Asset::FMeshImportResult::GetMeshes() {
        return mMeshes;
    }

    const Core::TArray<Asset::FMeshImportMesh>& Asset::FMeshImportResult::GetMeshes() const {
        return mMeshes;
    }

    Core::TArray<Asset::FMeshImportMaterial>& Asset::FMeshImportResult::GetMaterials() {
        return mMaterials;
    }

    const Core::TArray<Asset::FMeshImportMaterial>& Asset::FMeshImportResult::GetMaterials() const {
        return mMaterials;
    }

    Core::TArray<Asset::FMeshImportBone>& Asset::FMeshImportResult::GetBones() {
        return mBones;
    }

    const Core::TArray<Asset::FMeshImportBone>& Asset::FMeshImportResult::GetBones() const {
        return mBones;
    }

}
