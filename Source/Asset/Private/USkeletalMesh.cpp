/**
 * @file USkeletalMesh.cpp
 * @brief 본 데이터 검증과 공유 바인드 형상 및 GPU 스킨 가중치를 관리합니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace Asset {

    /**
     * @brief 형상과 가중치를 검증하고 저장합니다.
     */
    bool Asset::USkeletalMesh::SetMeshData(Asset::FMeshData MeshData, Core::TArray<Asset::FMeshImportSkinWeight> SkinWeights) {
        if (SkinWeights.size() != MeshData.GetPositions().size() || SkinWeights.size() > 16000000) {
            return false;
        }

        Core::Uint32 SkinMatrixCount{};

        for (const Asset::FMeshImportSkinWeight& SkinWeight : SkinWeights) {
            float Total{};

            for (Core::Index_t Index{}; Index < Asset::FMeshImportSkinWeight::MaxInfluences; ++Index) {
                const Core::Index_t BoneIndex{static_cast<Core::Index_t>(SkinWeight.GetBoneIndices()[Index])};
                const float Weight{SkinWeight.GetWeights()[Index]};

                if (BoneIndex < 0 || BoneIndex >= Asset::MaxSkeletonBones || !std::isfinite(Weight) || Weight < 0.0f || Weight > 1.0f) {
                    return false;
                }

                SkinMatrixCount = std::max(SkinMatrixCount, static_cast<Core::Uint32>(BoneIndex + 1));
                Total += Weight;
            }

            // 가중치 합은 1이어야 합니다. 합이 0인 정점은 본 영향이 없다는 뜻이며 바인드 위치에 그대로 남습니다.
            if (Total != 0.0f && std::abs(Total - 1.0f) > 0.0001f) {
                return false;
            }
        }

        if (!mGeometry.SetMeshData(std::move(MeshData))) {
            return false;
        }

        mSkinWeightBuffer.Reset();
        mSkinWeights = std::move(SkinWeights);
        mSkinMatrixCount = SkinMatrixCount;

        return true;
    }

    /**
     * @brief 바인드 포즈의 CPU 형상을 반환합니다.
     */
    const Asset::FMeshData& Asset::USkeletalMesh::GetMeshData() const {
        return mGeometry.GetMeshData();
    }

    /**
     * @brief 정점별 스킨 가중치를 반환합니다.
     */
    const Core::TArray<Asset::FMeshImportSkinWeight>& Asset::USkeletalMesh::GetSkinWeights() const {
        return mSkinWeights;
    }

    Core::Uint32 Asset::USkeletalMesh::GetSkinMatrixCount() const {
        return mSkinMatrixCount;
    }

    /**
     * @brief 공통 메시 드로 경로가 사용할 바인드 형상을 반환합니다.
     */
    Asset::UStaticMesh* Asset::USkeletalMesh::GetGeometry() {
        return &mGeometry;
    }

    /**
     * @brief 장치에 바인드 형상과 불변 가중치 버퍼를 준비합니다.
     */
    bool Asset::USkeletalMesh::CreateResources(ID3D11Device* Device) {
        if (Device == nullptr || !IsValid() || !mGeometry.CreateResources(Device)) {
            return false;
        }

        // 가중치는 에셋을 교체하기 전까지 변하지 않으므로, 같은 장치에 이미 만든 버퍼가 있으면 재사용합니다.
        if (mSkinWeightBuffer.IsCreatedOn(Device)) {
            return true;
        }

        // 임포트 클래스의 메모리 배치와 무관하게 uint4, float4 순으로 입력을 만듭니다.
        static_assert(sizeof(FSkinWeightVertex) == sizeof(Core::Uint32) * 4 + sizeof(float) * 4);

        Core::TArray<FSkinWeightVertex> Vertices{};
        Vertices.resize(mSkinWeights.size());

        for (Core::Index_t Index{}; Index < mSkinWeights.size(); ++Index) {
            std::memcpy(Vertices[Index].mBoneIndices.data(), mSkinWeights[Index].GetBoneIndices().data(), sizeof(Core::Uint32) * 4);
            std::memcpy(Vertices[Index].mWeights.data(), mSkinWeights[Index].GetWeights().data(), sizeof(float) * 4);
        }

        // 객체마다 달라지는 포즈는 본 팔레트(FScene::UploadSkinning)에 있고, 이 버퍼는 같은 에셋을 쓰는 객체가 공유하므로 불변으로 만듭니다.
        FSkinWeightBuffer Buffer{};
        Core::FString Error{};

        if (!Buffer.Create(Device, Vertices, Error)) {
            return false;
        }

        mSkinWeightBuffer = std::move(Buffer);

        return true;
    }

    /**
     * @brief 형상과 가중치의 GPU 자원을 해제합니다.
     */
    void Asset::USkeletalMesh::ReleaseResources() {
        mGeometry.ReleaseResources();
        mSkinWeightBuffer.Reset();
    }

    /**
     * @brief 본 인덱스와 가중치를 담은 정점 버퍼를 입력 슬롯에 연결합니다.
     */
    void Asset::USkeletalMesh::BindSkinWeights(ID3D11DeviceContext* Context, Core::Index_t Slot) const {
        mSkinWeightBuffer.Bind(Context, Slot);
    }

    /**
     * @brief 입력 슬롯의 스킨 가중치 버퍼 연결을 해제합니다.
     */
    void Asset::USkeletalMesh::UnbindSkinWeights(ID3D11DeviceContext* Context, Core::Index_t Slot) {
        FSkinWeightBuffer::Unbind(Context, Slot);
    }

    /**
     * @brief 바인드 형상의 로컬 경계를 반환합니다.
     */
    const Core::FBox& Asset::USkeletalMesh::GetBounds() const {
        return mGeometry.GetBounds();
    }

    /**
     * @brief 정점 수를 반환합니다.
     */
    Core::Uint32 Asset::USkeletalMesh::GetVertexCount() const {
        return mGeometry.GetVertexCount();
    }

    /**
     * @brief 인덱스 수를 반환합니다.
     */
    Core::Uint32 Asset::USkeletalMesh::GetIndexCount() const {
        return mGeometry.GetIndexCount();
    }

    /**
     * @brief 스키닝에 필요한 형상과 본 데이터가 있는지 검사합니다.
     */
    bool Asset::USkeletalMesh::IsValid() const {
        return mGeometry.IsValid() && mSkinWeights.size() == GetVertexCount();
    }

}
