/**
 * @file USkeletalMesh.h
 * @brief 바인드 형상과 본 계층 및 GPU 스킨 가중치를 공유하는 스켈레탈 메시 에셋입니다.
 * @ingroup Asset
 */
#pragma once

#include "Asset/FMeshData.h"
#include "Asset/UStaticMesh.h"
#include "Asset/Import/FMeshImportBone.h"
#include "Asset/Import/FMeshImportSkinWeight.h"
#include "Core/FRevision.h"
#include "CoreUObject/UObject.h"

namespace Asset {

    /** @brief 불변 바인드 형상과 본 데이터를 소유하며 현재 포즈는 각 컴포넌트가 관리합니다. */
    class USkeletalMesh final : public CoreUObject::UObject {
    public:
        ReflectTypes(USkeletalMesh, CoreUObject::UObject);

    public:
        /** @brief 빈 스켈레탈 메시를 생성합니다. */
        USkeletalMesh() = default;
        /** @brief 공유 GPU 자원을 해제합니다. */
        ~USkeletalMesh() override = default;

    public:
        /** @brief 스켈레탈 메시의 클래스 메타데이터를 반환합니다. @return 등록된 클래스입니다. */
        ReflectClass(USkeletalMesh);

        /** @brief 형상·부모 순서·가중치를 검증하고 역바인드 행렬을 계산합니다. @param MeshData 바인드 형상입니다. @param Bones 부모가 자식보다 앞선 본 목록입니다. @param SkinWeights 정점별 최대 네 본의 영향입니다. @return 모든 데이터가 유효해 교체했으면 true입니다. */
        bool SetMeshData(Asset::FMeshData MeshData, Core::TArray<Asset::FMeshImportBone> Bones, Core::TArray<Asset::FMeshImportSkinWeight> SkinWeights);

        /** @brief 바인드 포즈의 CPU 형상을 반환합니다. @return 공유 형상의 읽기 전용 참조입니다. */
        const Asset::FMeshData& GetMeshData() const;
        /** @brief 이름·부모·로컬 바인드 포즈를 반환합니다. @return 본 목록의 읽기 전용 참조입니다. */
        const Core::TArray<Asset::FMeshImportBone>& GetBones() const;
        /** @brief 정점별 스킨 가중치를 반환합니다. @return 가중치 목록의 읽기 전용 참조입니다. */
        const Core::TArray<Asset::FMeshImportSkinWeight>& GetSkinWeights() const;
        /** @brief 메시 공간에서 바인드 시점 본 공간으로 가는 행렬을 반환합니다. @return 본 인덱스순 역바인드 행렬입니다. */
        const Core::TArray<Core::FMatrix>& GetInverseBindPoses() const;

        /** @brief 공통 메시 드로 경로가 사용할 바인드 형상을 반환합니다. @return 에셋 수명 동안 주소가 유지되는 형상입니다. */
        Asset::UStaticMesh* GetGeometry();
        /** @brief 장치에 바인드 형상과 불변 가중치 버퍼를 준비합니다. @param Device 사용할 D3D11 장치입니다. @return 모든 버퍼를 준비하면 true입니다. */
        bool CreateResources(ID3D11Device* Device);
        /** @brief 형상과 가중치의 GPU 자원을 해제합니다. */
        void ReleaseResources();
        /** @brief 본 인덱스 네 개와 가중치 네 개를 담은 정점 버퍼를 반환합니다. @return GPU 자원이 없으면 nullptr입니다. */
        ID3D11Buffer* GetSkinWeightBuffer() const;
        /** @brief GPU 스킨 정점의 바이트 간격을 반환합니다. @return uint4와 float4를 합한 크기입니다. */
        static Core::Uint32 GetSkinWeightStride();

        /** @brief 바인드 형상의 로컬 경계를 반환합니다. @return 공유 경계 상자입니다. */
        const Core::FBox& GetBounds() const;
        /** @brief 정점 수를 반환합니다. @return 바인드 형상의 정점 수입니다. */
        Core::Uint32 GetVertexCount() const;
        /** @brief 인덱스 수를 반환합니다. @return 삼각형 인덱스 수입니다. */
        Core::Uint32 GetIndexCount() const;
        /** @brief 에셋 데이터의 변경 식별자를 반환합니다. @return 마지막 데이터 교체 시 발급한 개정입니다. */
        Core::FRevision GetRevision() const;
        /** @brief 스키닝에 필요한 형상과 본 데이터가 있는지 검사합니다. @return 유효한 메시이면 true입니다. */
        bool IsValid() const;

    private:
        ReflectMembers();

    private:
        /** @brief 정적 메시와 공유하는 버퍼 구현이며 위치는 바인드 포즈를 유지합니다. */
        Asset::UStaticMesh mGeometry{};
        ReflectProperty(mGeometry, "Geometry", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /** @brief 부모 인덱스와 로컬 바인드 포즈입니다. */
        Core::TArray<Asset::FMeshImportBone> mBones{};
        ReflectProperty(mBones, "Bones", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /** @brief 정점별 정규화한 본 영향입니다. */
        Core::TArray<Asset::FMeshImportSkinWeight> mSkinWeights{};
        ReflectProperty(mSkinWeights, "SkinWeights", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /** @brief 메시 공간 기준 전역 바인드 포즈의 역행렬입니다. */
        Core::TArray<Core::FMatrix> mInverseBindPoses{};
        ReflectProperty(mInverseBindPoses, "InverseBindPoses", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /** @brief 바인드 데이터 개정입니다. */
        Core::FRevision mRevision{};
        ReflectProperty(mRevision, "Revision", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /** @brief BLENDINDICES와 BLENDWEIGHT 입력의 공유 불변 버퍼입니다. */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mSkinWeightBuffer{};
        ReflectProperty(mSkinWeightBuffer, "SkinWeightBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
