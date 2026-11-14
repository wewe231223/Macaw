/**
 * @file FSkeletalMeshPose.cpp
 * @brief 부모 계층과 역바인드 행렬로 현재 포즈 및 CPU 선택 형상을 계산합니다.
 * @ingroup Engine
 */

#include "Core/Types.h"
#include "Engine/FSkeletalMeshPose.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Engine {

    /**
     * @brief 에셋의 바인드 포즈로 초기화합니다.
     */
    bool FSkeletalMeshPose::Reset(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton) {
        Core::TArray<Core::FMatrix> LocalTransforms{};
        LocalTransforms.reserve(Skeleton.GetBones().size());

        // 로컬 바인드 포즈를 현재 포즈로 쓰면 모든 스키닝 행렬이 항등 행렬이 되어 메시가 바인드 형상 그대로 나타납니다.
        for (const Asset::FMeshImportBone& Bone : Skeleton.GetBones()) {
            LocalTransforms.push_back(Bone.GetBindPose());
        }

        return SetLocalTransforms(Mesh, Skeleton, LocalTransforms);
    }

    /**
     * @brief 로컬 포즈와 스켈레톤으로 스킨 행렬·변형 정점·경계를 다시 계산합니다.
     */
    bool FSkeletalMeshPose::Recalculate(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton) {
        return SetLocalTransforms(Mesh, Skeleton, mLocalTransforms);
    }

    /**
     * @brief 스켈레탈 포즈의 계산 결과를 다시 만들도록 표시합니다.
     */
    void FSkeletalMeshPose::Invalidate() {
        mGlobalTransforms.clear();
        mSkinMatrices.clear();
        mMeshData = Asset::FMeshData{};
        mRevision = Core::FRevision{};
    }

    /**
     * @brief 로컬 포즈를 검증하고 변형 형상을 계산하여 반영합니다.
     */
    bool FSkeletalMeshPose::SetLocalTransforms(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton, const Core::TArray<Core::FMatrix>& LocalTransforms) {
        if (!Mesh.IsValid() || !Skeleton.IsValid() || LocalTransforms.size() != Skeleton.GetBones().size()) {
            return false;
        }

        Core::TArray<Core::FMatrix> Globals{};
        Core::TArray<Core::FMatrix> Skins{};

        Globals.reserve(LocalTransforms.size());
        Skins.reserve(LocalTransforms.size());

        // 행 벡터 규약에서는 자식 로컬 변환을 부모 전역 변환의 왼쪽에 곱합니다.
        // 부모가 자식보다 앞에 있으므로 Globals[Parent]는 이 시점에 항상 계산되어 있습니다.
        for (Core::Index_t Index{}; Index < LocalTransforms.size(); ++Index) {
            const Core::FMatrix& Local{LocalTransforms[Index]};
            const Core::Index_t Parent{static_cast<Core::Index_t>(Skeleton.GetBones()[Index].GetParentIndex())};
            const Core::FMatrix Global{Parent < 0 ? Local : Local * Globals[Parent]};

            // 스키닝 행렬: 바인드 정점을 역바인드 행렬로 본 공간에 옮긴 뒤, 현재 전역 포즈로 다시 메시 공간에 옮깁니다.
            const Core::FMatrix Skin{Skeleton.GetInverseBindPoses()[Index] * Global};
            Core::FMatrix Inverse{};

            // 투영 변환과 특이 행렬은 위치·법선 계산을 일치시킬 수 없어 거부합니다.
            // 행 벡터 규약의 아핀 행렬은 4번째 열이 (0, 0, 0, 1)이어야 합니다.
            if (!Local.IsFinite() || std::abs(Local(0, 3)) > 0.00001f || std::abs(Local(1, 3)) > 0.00001f || std::abs(Local(2, 3)) > 0.00001f || std::abs(Local(3, 3) - 1.0f) > 0.00001f || !Skin.IsFinite() || !Skin.TryInverse(Inverse) || !Inverse.IsFinite()) {
                return false;
            }

            Globals.push_back(Global);
            Skins.push_back(Skin);
        }

        if (Skins.size() < Mesh.GetSkinMatrixCount()) {
            Skins.resize(Mesh.GetSkinMatrixCount(), Core::FMatrix::Identity());
        }

        Asset::FMeshData Deformed{Mesh.GetMeshData()};

        // GPU와 같은 가중합으로 위치를 계산하여 실제 변형된 표면을 선택할 수 있게 합니다.
        // 렌더링 자체는 GPU가 맡으므로 CPU 사본에서는 피킹과 경계 계산에 필요한 위치만 갱신합니다.
        for (Core::Index_t Vertex{}; Vertex < Deformed.GetPositions().size(); ++Vertex) {
            const Core::FVector Position{Mesh.GetMeshData().GetPositions()[Vertex]};
            const Asset::FMeshImportSkinWeight& Influence{Mesh.GetSkinWeights()[Vertex]};
            Core::FVector Result{};
            float Total{};

            for (Core::Index_t Slot{}; Slot < Asset::FMeshImportSkinWeight::MaxInfluences; ++Slot) {
                // 쓰지 않는 칸은 본 인덱스 0, 가중치 0이므로 결과에 영향을 주지 않습니다.
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

        // 모든 검증을 통과한 뒤에만 멤버를 교체하므로, 실패하면 이전 포즈가 그대로 유지됩니다.
        Deformed.ComputeBounds();

        mLocalTransforms = LocalTransforms;
        mGlobalTransforms = std::move(Globals);
        mSkinMatrices = std::move(Skins);
        mMeshData = std::move(Deformed);
        mRevision = Core::FRevision::Next();

        return true;
    }

    bool FSkeletalMeshPose::SetAnimatedLocalTransforms(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton, Core::TArray<Core::FMatrix> LocalTransforms) {
        const auto& Bones{Skeleton.GetBones()};

        if (LocalTransforms.size() != Bones.size()) {
            return false;
        }

        mGlobalTransforms.resize(Bones.size());
        mSkinMatrices.resize(std::max<std::size_t>(Bones.size(), Mesh.GetSkinMatrixCount()), Core::FMatrix::Identity());

        // 부모가 항상 자식보다 앞에 있으므로 한 번의 순회로 전역 행렬과 스키닝 행렬을 구합니다.
        for (Core::Index_t Index{}; Index < Bones.size(); ++Index) {
            const Core::Index_t Parent{static_cast<Core::Index_t>(Bones[Index].GetParentIndex())};

            mGlobalTransforms[Index] = Parent < 0 ? LocalTransforms[Index] : LocalTransforms[Index] * mGlobalTransforms[Parent];
            mSkinMatrices[Index] = Skeleton.GetInverseBindPoses()[Index] * mGlobalTransforms[Index];
        }

        mLocalTransforms = std::move(LocalTransforms);
        mRevision = Core::FRevision::Next();

        return true;
    }

    /**
     * @brief 본별 로컬 변환을 검증해 저장하고 이전 메시 계산 결과를 무효화합니다.
     */
    bool FSkeletalMeshPose::SetLocalTransforms(Core::TArray<Core::FMatrix> LocalTransforms) {
        for (const Core::FMatrix& Local : LocalTransforms) {
            Core::FMatrix Inverse{};

            if (!Local.IsFinite() || std::abs(Local(0, 3)) > 0.00001f || std::abs(Local(1, 3)) > 0.00001f || std::abs(Local(2, 3)) > 0.00001f || std::abs(Local(3, 3) - 1.0f) > 0.00001f || !Local.TryInverse(Inverse) || !Inverse.IsFinite()) {
                return false;
            }
        }

        mLocalTransforms = std::move(LocalTransforms);

        Invalidate();

        return true;
    }

    /**
     * @brief 본 하나의 로컬 변환을 교체합니다.
     */
    bool FSkeletalMeshPose::SetLocalTransform(const Asset::USkeletalMesh& Mesh, const Asset::USkeleton& Skeleton, Core::Index_t Bone, const Core::FMatrix& Transform) {
        if (Bone >= mLocalTransforms.size()) {
            return false;
        }

        // 본 하나가 바뀌면 그 자손의 전역 변환도 달라지므로 전체 포즈를 다시 계산합니다.
        Core::TArray<Core::FMatrix> Transforms{mLocalTransforms};
        Transforms[Bone] = Transform;

        return SetLocalTransforms(Mesh, Skeleton, Transforms);
    }

    /**
     * @brief 로컬 포즈를 반환합니다.
     */
    const Core::TArray<Core::FMatrix>& FSkeletalMeshPose::GetLocalTransforms() const {
        return mLocalTransforms;
    }

    /**
     * @brief 메시 공간의 현재 본 변환을 반환합니다.
     */
    const Core::TArray<Core::FMatrix>& FSkeletalMeshPose::GetGlobalTransforms() const {
        return mGlobalTransforms;
    }

    /**
     * @brief 역바인드 행렬에 현재 전역 포즈를 곱한 결과를 반환합니다.
     */
    const Core::TArray<Core::FMatrix>& FSkeletalMeshPose::GetSkinMatrices() const {
        return mSkinMatrices;
    }

    /**
     * @brief 현재 포즈로 변형된 CPU 형상을 반환합니다.
     */
    const Asset::FMeshData& FSkeletalMeshPose::GetMeshData() const {
        return mMeshData;
    }

    /**
     * @brief 포즈의 변경 식별자를 반환합니다.
     */
    Core::FRevision FSkeletalMeshPose::GetRevision() const {
        return mRevision;
    }

}
