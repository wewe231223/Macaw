/**
 * @file USkeletalMesh.h
 * @brief 바인드 형상과 GPU 스킨 가중치를 공유하는 스켈레탈 메시 에셋입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Asset/FMeshData.h"
#include "Asset/UStaticMesh.h"
#include "Asset/Import/FMeshImportSkinWeight.h"
#include "CoreUObject/UObject.h"
#include "RenderCore/TVertexBuffer.h"

#include <array>
#include "USkeletalMesh.generated.h"

namespace Asset {

    /**
     * @brief 불변 바인드 형상과 가중치를 소유하며 현재 포즈는 각 컴포넌트가 관리합니다.
     * @details 스키닝 데이터는 다음 순서로 흐릅니다.
     * 1. USkeleton이 로컬 바인드 포즈를 누적해 역바인드 행렬을 계산합니다.
     * 2. FSkeletalMeshPose가 현재 로컬 포즈를 누적해 전역 포즈를 구하고, 스키닝 행렬(역바인드 * 전역)을 계산합니다.
     * 3. USkeletalMeshComponent가 스키닝 행렬을 씬 프록시에 복사하고, FScene::UploadSkinning이 본 팔레트로 GPU에 올립니다.
     * 4. 정점 셰이더(Skinning.hlsl)가 가중치로 섞은 행렬을 적용해 정점을 현재 포즈로 옮깁니다.
     *
     * 행 벡터 규약의 최종 위치는 v' = Σ 가중치(i) * (v * 역바인드(i) * 전역(i))이며, 바인드 포즈에서는 스키닝 행렬이 항등 행렬이 되어 v' = v입니다.
     */
    UCLASS()
    class USkeletalMesh final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:

        /**
         * @brief 빈 스켈레탈 메시를 생성합니다.
         */
        USkeletalMesh() = default;

        /**
         * @brief 공유 GPU 자원을 해제합니다.
         */
        ~USkeletalMesh() override = default;

    public:

        /**
         * @brief 형상과 가중치를 검증하고 저장합니다.
         * @param MeshData 바인드 형상입니다.
         * @param SkinWeights 정점별 최대 네 본의 영향입니다.
         * @return 모든 데이터가 유효해 교체했으면 true입니다.
         */
        bool SetMeshData(Asset::FMeshData MeshData, Core::TArray<Asset::FMeshImportSkinWeight> SkinWeights);

        /**
         * @brief 바인드 포즈의 CPU 형상을 반환합니다.
         * @return 공유 형상의 읽기 전용 참조입니다.
         */
        const Asset::FMeshData& GetMeshData() const;

        /**
         * @brief 정점별 스킨 가중치를 반환합니다.
         * @return 가중치 목록의 읽기 전용 참조입니다.
         */
        const Core::TArray<Asset::FMeshImportSkinWeight>& GetSkinWeights() const;

        /**
         * @brief 스키닝 가중치가 요구하는 본 행렬 수를 반환합니다.
         * @return GPU 스키닝에 필요한 행렬 개수입니다.
         */
        Core::Uint32 GetSkinMatrixCount() const;

        /**
         * @brief 공통 메시 드로 경로가 사용할 바인드 형상을 반환합니다.
         * @return 에셋 수명 동안 주소가 유지되는 형상입니다.
         */
        Asset::UStaticMesh* GetGeometry();

        /**
         * @brief 장치에 바인드 형상과 불변 가중치 버퍼를 준비합니다.
         * @param Device 사용할 D3D11 장치입니다.
         * @return 모든 버퍼를 준비하면 true입니다.
         */
        bool CreateResources(ID3D11Device* Device);

        /**
         * @brief 형상과 가중치의 GPU 자원을 해제합니다.
         */
        void ReleaseResources();

        /**
         * @brief 본 인덱스와 가중치를 담은 정점 버퍼를 입력 슬롯에 연결합니다.
         * @param Context 바인딩할 장치 컨텍스트입니다.
         * @param Slot 입력 슬롯입니다.
         */
        void BindSkinWeights(ID3D11DeviceContext* Context, Core::Index_t Slot) const;

        /**
         * @brief 입력 슬롯의 스킨 가중치 버퍼 연결을 해제합니다.
         * @param Context 바인딩할 장치 컨텍스트입니다.
         * @param Slot 입력 슬롯입니다.
         */
        static void UnbindSkinWeights(ID3D11DeviceContext* Context, Core::Index_t Slot);

        /**
         * @brief 바인드 형상의 로컬 경계를 반환합니다.
         * @return 공유 경계 상자입니다.
         */
        const Core::FBox& GetBounds() const;

        /**
         * @brief 정점 수를 반환합니다.
         * @return 바인드 형상의 정점 수입니다.
         */
        Core::Uint32 GetVertexCount() const;

        /**
         * @brief 인덱스 수를 반환합니다.
         * @return 삼각형 인덱스 수입니다.
         */
        Core::Uint32 GetIndexCount() const;

        /**
         * @brief 스키닝에 필요한 형상과 가중치가 있는지 검사합니다.
         * @return 유효한 메시이면 true입니다.
         */
        bool IsValid() const;

    private:

        /**
         * @brief BLENDINDICES(uint4)와 BLENDWEIGHT(float4) 입력 하나의 GPU 배치입니다.
         */
        struct FSkinWeightVertex {

            /**
             * @brief 정점에 영향을 주는 최대 네 개의 본 인덱스입니다.
             */
            std::array<Core::Uint32, 4> mBoneIndices{};

            /**
             * @brief 본 인덱스 순서에 대응하는 정점 스킨 가중치입니다.
             */
            std::array<float, 4> mWeights{};
        };

        /**
         * @brief 스킨 가중치를 담는 불변 정점 버퍼입니다.
         */
        using FSkinWeightBuffer = RenderCore::TVertexBuffer<FSkinWeightVertex, RenderCore::EBufferUpdate::Immutable>;

    private:

        /**
         * @brief 정적 메시와 공유하는 버퍼 구현이며 위치는 바인드 포즈를 유지합니다.
         */
        Asset::UStaticMesh mGeometry{};

        /**
         * @brief 정점별 정규화한 본 영향입니다.
         */
        Core::TArray<Asset::FMeshImportSkinWeight> mSkinWeights{};

        /**
         * @brief GPU 스키닝에서 참조할 본 행렬 수입니다.
         */
        Core::Uint32 mSkinMatrixCount{};

        /**
         * @brief BLENDINDICES와 BLENDWEIGHT 입력의 공유 불변 버퍼입니다.
         */
        FSkinWeightBuffer mSkinWeightBuffer{};
    };

}
