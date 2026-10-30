/**
 * @file FMeshImportResult.cpp
 * @brief 정점·인덱스·서브메시와 원본 재질 목록을 함께 보관하는 메시 임포트 결과입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportResult.h"

namespace Asset {

    Asset::FMeshData& Asset::FMeshImportResult::GetMeshData() {
        return mMeshData;
    }

    const Asset::FMeshData& Asset::FMeshImportResult::GetMeshData() const {
        return mMeshData;
    }

    Core::TArray<Asset::FMeshImportMaterial>& Asset::FMeshImportResult::GetMaterials() {
        return mMaterials;
    }

    const Core::TArray<Asset::FMeshImportMaterial>& Asset::FMeshImportResult::GetMaterials() const {
        return mMaterials;
    }

}
