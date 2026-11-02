/**
 * @file USkeletalMesh.cpp
 * @brief 본 데이터 검증과 공유 바인드 형상 및 GPU 스킨 가중치를 관리합니다.
 * @ingroup Asset
 */
#include "Asset/USkeletalMesh.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace Asset {

    bool Asset::USkeletalMesh::SetMeshData(Asset::FMeshData MeshData, Core::TArray<Asset::FMeshImportBone> Bones, Core::TArray<Asset::FMeshImportSkinWeight> SkinWeights) {
        if (Bones.empty() || Bones.size() > 100000 || SkinWeights.size() != MeshData.GetPositions().size() || SkinWeights.size() > 16000000) {
            return false;
        }

        Core::TArray<Core::FMatrix> GlobalBindPoses{};
        Core::TArray<Core::FMatrix> InverseBindPoses{};
        GlobalBindPoses.reserve(Bones.size());
        InverseBindPoses.reserve(Bones.size());

        // 부모가 자식보다 앞에 있으므로, 앞에서부터 로컬 바인드 포즈를 누적하면 메시 공간의 전역 바인드 포즈를 얻습니다.
        for (std::size_t Index{}; Index < Bones.size(); ++Index) {
            const Asset::FMeshImportBone& Bone{Bones[Index]};
            const Core::Int32 Parent{Bone.GetParentIndex()};

            // 부모 인덱스는 -1(루트)이거나 자신보다 작아야 합니다. 이 순서 덕분에 아래 누적과 포즈 계산이 한 번의 순회로 끝납니다.
            if (Parent < -1 || Parent >= static_cast<Core::Int32>(Index) || Bone.GetName().find('\0') != Core::FString::npos || !Bone.GetBindPose().IsFinite()) {
                return false;
            }

            const Core::FMatrix Global{Parent < 0 ? Bone.GetBindPose() : Bone.GetBindPose() * GlobalBindPoses[static_cast<std::size_t>(Parent)]};
            // 역바인드 행렬은 메시 공간의 바인드 정점을 이 본의 로컬 공간으로 옮깁니다. 본을 기준으로 한 정점의 상대 위치를 얻는 단계입니다.
            Core::FMatrix Inverse{};

            if (!Global.IsFinite() || !Global.TryInverse(Inverse) || !Inverse.IsFinite()) {
                return false;
            }

            GlobalBindPoses.push_back(Global);
            InverseBindPoses.push_back(Inverse);
        }

        for (const Asset::FMeshImportSkinWeight& SkinWeight : SkinWeights) {
            float Total{};

            for (std::size_t Index{}; Index < Asset::FMeshImportSkinWeight::MaxInfluences; ++Index) {
                const float Weight{SkinWeight.GetWeights()[Index]};

                if (SkinWeight.GetBoneIndices()[Index] >= Bones.size() || !std::isfinite(Weight) || Weight < 0.0f || Weight > 1.0f) {
                    return false;
                }

                Total += Weight;
            }

            // 가중치 합은 1이어야 합니다. 합이 0인 정점은 본 영향이 없다는 뜻이며 바인드 위치에 그대로 남습니다.
            if (Total != 0.0f && std::abs(Total - 1.0f) > 0.0001f) {
                return false;
            }
        }

        // 형상 검증은 내부 메시 설정에서 한 번 수행하며 실패하면 기존 본 데이터도 유지합니다.
        if (!mGeometry.SetMeshData(std::move(MeshData))) {
            return false;
        }

        mSkinWeightBuffer.Reset();
        mBones = std::move(Bones);
        mSkinWeights = std::move(SkinWeights);
        mInverseBindPoses = std::move(InverseBindPoses);
        mRevision = Core::FRevision::Next();
        return true;
    }

    const Asset::FMeshData& Asset::USkeletalMesh::GetMeshData() const {
        return mGeometry.GetMeshData();
    }

    const Core::TArray<Asset::FMeshImportBone>& Asset::USkeletalMesh::GetBones() const {
        return mBones;
    }

    const Core::TArray<Asset::FMeshImportSkinWeight>& Asset::USkeletalMesh::GetSkinWeights() const {
        return mSkinWeights;
    }

    const Core::TArray<Core::FMatrix>& Asset::USkeletalMesh::GetInverseBindPoses() const {
        return mInverseBindPoses;
    }

    Asset::UStaticMesh* Asset::USkeletalMesh::GetGeometry() {
        return &mGeometry;
    }

    bool Asset::USkeletalMesh::CreateResources(ID3D11Device* Device) {
        if (Device == nullptr || !IsValid() || !mGeometry.CreateResources(Device)) {
            return false;
        }

        // 가중치는 에셋을 교체하기 전까지 변하지 않으므로, 같은 장치에 이미 만든 버퍼가 있으면 재사용합니다.
        if (mSkinWeightBuffer != nullptr) {
            Microsoft::WRL::ComPtr<ID3D11Device> ExistingDevice{};
            mSkinWeightBuffer->GetDevice(ExistingDevice.GetAddressOf());

            if (ExistingDevice.Get() == Device) {
                return true;
            }
        }

        // 임포트 클래스의 메모리 배치와 무관하게 uint4, float4 순으로 입력을 만듭니다.
        const std::size_t Bytes{mSkinWeights.size() * GetSkinWeightStride()};

        if (Bytes > std::numeric_limits<UINT>::max()) {
            return false;
        }

        Core::TArray<Core::Uint8> Vertices{};
        Vertices.resize(Bytes);

        for (std::size_t Index{}; Index < mSkinWeights.size(); ++Index) {
            Core::Uint8* Target{Vertices.data() + Index * GetSkinWeightStride()};
            std::memcpy(Target, mSkinWeights[Index].GetBoneIndices().data(), sizeof(Core::Uint32) * 4);
            std::memcpy(Target + sizeof(Core::Uint32) * 4, mSkinWeights[Index].GetWeights().data(), sizeof(float) * 4);
        }

        // 객체마다 달라지는 포즈는 본 팔레트(FScene::UploadSkinning)에 있고, 이 버퍼는 같은 에셋을 쓰는 객체가 공유하므로 불변으로 만듭니다.
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = static_cast<UINT>(Bytes);
        Description.Usage = D3D11_USAGE_IMMUTABLE;
        Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA Data{};
        Data.pSysMem = Vertices.data();
        Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer{};

        if (FAILED(Device->CreateBuffer(&Description, &Data, Buffer.GetAddressOf()))) {
            return false;
        }

        mSkinWeightBuffer = std::move(Buffer);
        return true;
    }

    void Asset::USkeletalMesh::ReleaseResources() {
        mGeometry.ReleaseResources();
        mSkinWeightBuffer.Reset();
    }

    ID3D11Buffer* Asset::USkeletalMesh::GetSkinWeightBuffer() const {
        return mSkinWeightBuffer.Get();
    }

    Core::Uint32 Asset::USkeletalMesh::GetSkinWeightStride() {
        return sizeof(Core::Uint32) * 4 + sizeof(float) * 4;
    }

    const Core::FBox& Asset::USkeletalMesh::GetBounds() const {
        return mGeometry.GetBounds();
    }

    Core::Uint32 Asset::USkeletalMesh::GetVertexCount() const {
        return mGeometry.GetVertexCount();
    }

    Core::Uint32 Asset::USkeletalMesh::GetIndexCount() const {
        return mGeometry.GetIndexCount();
    }

    Core::FRevision Asset::USkeletalMesh::GetRevision() const {
        return mRevision;
    }

    bool Asset::USkeletalMesh::IsValid() const {
        return mGeometry.IsValid() && !mBones.empty() && mSkinWeights.size() == GetVertexCount() && mInverseBindPoses.size() == mBones.size();
    }

}
