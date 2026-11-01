/**
 * @file FSkeletalMeshPose.cpp
 * @brief 부모 계층과 역바인드 행렬로 현재 포즈 및 CPU 선택 형상을 계산합니다.
 * @ingroup Engine
 */
#include "Engine/FSkeletalMeshPose.h"
#include "Asset/USkeletalMesh.h"

#include <cmath>
#include <utility>

namespace Engine {

    bool FSkeletalMeshPose::Reset(const Asset::USkeletalMesh& Mesh) {
        Core::TArray<Core::FMatrix> LocalTransforms{};
        LocalTransforms.reserve(Mesh.GetBones().size());

        for (const Asset::FMeshImportBone& Bone : Mesh.GetBones()) {
            LocalTransforms.push_back(Bone.GetBindPose());
        }

        return SetLocalTransforms(Mesh, LocalTransforms);
    }

    bool FSkeletalMeshPose::SetLocalTransforms(const Asset::USkeletalMesh& Mesh, const Core::TArray<Core::FMatrix>& LocalTransforms) {
        if (!Mesh.IsValid() || LocalTransforms.size() != Mesh.GetBones().size()) {
            return false;
        }

        Core::TArray<Core::FMatrix> Globals{};
        Core::TArray<Core::FMatrix> Skins{};
        Globals.reserve(LocalTransforms.size());
        Skins.reserve(LocalTransforms.size());

        // 행 벡터 규약에서는 자식 로컬 변환을 부모 전역 변환의 왼쪽에 곱합니다.
        for (std::size_t Index{}; Index < LocalTransforms.size(); ++Index) {
            const Core::FMatrix& Local{LocalTransforms[Index]};
            const Core::Int32 Parent{Mesh.GetBones()[Index].GetParentIndex()};
            const Core::FMatrix Global{Parent < 0 ? Local : Local * Globals[static_cast<std::size_t>(Parent)]};
            const Core::FMatrix Skin{Mesh.GetInverseBindPoses()[Index] * Global};
            Core::FMatrix Inverse{};

            // 투영 변환과 특이 행렬은 위치·법선 계산을 일치시킬 수 없어 거부합니다.
            if (!Local.IsFinite() || std::abs(Local(0, 3)) > 0.00001f || std::abs(Local(1, 3)) > 0.00001f || std::abs(Local(2, 3)) > 0.00001f || std::abs(Local(3, 3) - 1.0f) > 0.00001f || !Skin.IsFinite() || !Skin.TryInverse(Inverse) || !Inverse.IsFinite()) {
                return false;
            }

            Globals.push_back(Global);
            Skins.push_back(Skin);
        }

        Asset::FMeshData Deformed{Mesh.GetMeshData()};

        // GPU와 같은 가중합으로 위치를 계산하여 실제 변형된 표면을 선택할 수 있게 합니다.
        for (std::size_t Vertex{}; Vertex < Deformed.GetPositions().size(); ++Vertex) {
            const Core::FVector Position{Mesh.GetMeshData().GetPositions()[Vertex]};
            const Asset::FMeshImportSkinWeight& Influence{Mesh.GetSkinWeights()[Vertex]};
            Core::FVector Result{};
            float Total{};

            for (std::size_t Slot{}; Slot < Asset::FMeshImportSkinWeight::MaxInfluences; ++Slot) {
                const float Weight{Influence.GetWeights()[Slot]};
                Result += Skins[Influence.GetBoneIndices()[Slot]].TransformPosition(Position) * Weight;
                Total += Weight;
            }

            // 본 영향이 없는 정점은 원래 형상을 유지합니다.
            Deformed.GetPositions()[Vertex] = Total > 0.0f ? Result / Total : Position;

            if (!Deformed.GetPositions()[Vertex].IsFinite()) {
                return false;
            }
        }

        Deformed.ComputeBounds();
        mLocalTransforms = LocalTransforms;
        mGlobalTransforms = std::move(Globals);
        mSkinMatrices = std::move(Skins);
        mMeshData = std::move(Deformed);
        mMeshRevision = Mesh.GetRevision();
        mRevision = Core::FRevision::Next();
        return true;
    }

    bool FSkeletalMeshPose::SetLocalTransform(const Asset::USkeletalMesh& Mesh, Core::Uint32 Bone, const Core::FMatrix& Transform) {
        if (Bone >= mLocalTransforms.size() || mMeshRevision != Mesh.GetRevision()) {
            return false;
        }

        Core::TArray<Core::FMatrix> Transforms{mLocalTransforms};
        Transforms[Bone] = Transform;
        return SetLocalTransforms(Mesh, Transforms);
    }

    const Core::TArray<Core::FMatrix>& FSkeletalMeshPose::GetLocalTransforms() const {
        return mLocalTransforms;
    }

    const Core::TArray<Core::FMatrix>& FSkeletalMeshPose::GetGlobalTransforms() const {
        return mGlobalTransforms;
    }

    const Core::TArray<Core::FMatrix>& FSkeletalMeshPose::GetSkinMatrices() const {
        return mSkinMatrices;
    }

    const Asset::FMeshData& FSkeletalMeshPose::GetMeshData() const {
        return mMeshData;
    }

    Core::FRevision FSkeletalMeshPose::GetRevision() const {
        return mRevision;
    }

    Core::FRevision FSkeletalMeshPose::GetMeshRevision() const {
        return mMeshRevision;
    }

}
