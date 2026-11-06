/**
 * @file FMeshImportResult.cpp
 * @brief 임포트한 메시 목록과 원본 재질·스켈레톤 본 목록을 함께 보관하는 메시 임포트 결과입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportResult.h"

namespace Asset {

    /**
     * @brief 임포트한 메시 목록을 조회합니다. 메시 병합 옵션을 켜면 항상 하나입니다.
     */
    Core::TArray<Asset::FMeshImportMesh>& Asset::FMeshImportResult::GetMeshes() {
        return mMeshes;
    }

    /**
     * @brief 임포트한 메시 목록을 조회합니다. 메시 병합 옵션을 켜면 항상 하나입니다.
     */
    const Core::TArray<Asset::FMeshImportMesh>& Asset::FMeshImportResult::GetMeshes() const {
        return mMeshes;
    }

    /**
     * @brief 메시 재질 슬롯이 인덱스로 가리키는 재질 목록을 조회합니다.
     */
    Core::TArray<Asset::FMeshImportMaterial>& Asset::FMeshImportResult::GetMaterials() {
        return mMaterials;
    }

    /**
     * @brief 메시 재질 슬롯이 인덱스로 가리키는 재질 목록을 조회합니다.
     */
    const Core::TArray<Asset::FMeshImportMaterial>& Asset::FMeshImportResult::GetMaterials() const {
        return mMaterials;
    }

    /**
     * @brief 스켈레톤 임포트 옵션을 켰을 때 읽은 본 목록을 조회합니다. 부모 본은 항상 자식 본보다 앞에 있습니다.
     */
    Core::TArray<Asset::FMeshImportBone>& Asset::FMeshImportResult::GetBones() {
        return mBones;
    }

    /**
     * @brief 스켈레톤 임포트 옵션을 켰을 때 읽은 본 목록을 조회합니다. 부모 본은 항상 자식 본보다 앞에 있습니다.
     */
    const Core::TArray<Asset::FMeshImportBone>& Asset::FMeshImportResult::GetBones() const {
        return mBones;
    }

}
