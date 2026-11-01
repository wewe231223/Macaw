/**
 * @file FMeshImportResult.h
 * @brief 임포트한 메시 목록과 원본 재질·스켈레톤 본 목록을 함께 보관하는 메시 임포트 결과입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Import/FMeshImportBone.h"
#include "Asset/Import/FMeshImportMaterial.h"
#include "Asset/Import/FMeshImportMesh.h"

namespace Asset {

    /**
     * @brief 임포트한 메시 목록과 원본 재질·스켈레톤 본 목록을 함께 보관하는 메시 임포트 결과입니다.
     */
    class FMeshImportResult {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FMeshImportResult);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshImportResult);

        /**
         * @brief 임포트한 메시 목록을 조회합니다. 메시 병합 옵션을 켜면 항상 하나입니다.
         * @return 결과 객체가 소유하는 메시 목록의 참조입니다.
         */
        Core::TArray<Asset::FMeshImportMesh>& GetMeshes();
        /**
         * @brief 임포트한 메시 목록을 조회합니다. 메시 병합 옵션을 켜면 항상 하나입니다.
         * @return 결과 객체가 소유하는 메시 목록의 참조입니다.
         */
        const Core::TArray<Asset::FMeshImportMesh>& GetMeshes() const;

        /**
         * @brief 메시 재질 슬롯이 인덱스로 가리키는 재질 목록을 조회합니다.
         * @return 결과 객체가 소유하는 재질 목록의 참조입니다.
         */
        Core::TArray<Asset::FMeshImportMaterial>& GetMaterials();
        /**
         * @brief 메시 재질 슬롯이 인덱스로 가리키는 재질 목록을 조회합니다.
         * @return 결과 객체가 소유하는 재질 목록의 참조입니다.
         */
        const Core::TArray<Asset::FMeshImportMaterial>& GetMaterials() const;

        /**
         * @brief 스켈레톤 임포트 옵션을 켰을 때 읽은 본 목록을 조회합니다. 부모 본은 항상 자식 본보다 앞에 있습니다.
         * @return 결과 객체가 소유하는 본 목록의 참조입니다.
         */
        Core::TArray<Asset::FMeshImportBone>& GetBones();
        /**
         * @brief 스켈레톤 임포트 옵션을 켰을 때 읽은 본 목록을 조회합니다. 부모 본은 항상 자식 본보다 앞에 있습니다.
         * @return 결과 객체가 소유하는 본 목록의 참조입니다.
         */
        const Core::TArray<Asset::FMeshImportBone>& GetBones() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 임포트한 메시 목록입니다.
         */
        Core::TArray<Asset::FMeshImportMesh> mMeshes{};
        ReflectProperty(mMeshes, "Meshes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시 재질 슬롯이 인덱스로 가리키는 원본 재질 설정 목록입니다.
         */
        Core::TArray<Asset::FMeshImportMaterial> mMaterials{};
        ReflectProperty(mMaterials, "Materials", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원본 스켈레톤 계층에서 읽은 본 목록입니다.
         */
        Core::TArray<Asset::FMeshImportBone> mBones{};
        ReflectProperty(mBones, "Bones", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
