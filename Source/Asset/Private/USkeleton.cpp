#include "Asset/USkeleton.h"

#include <utility>

namespace Asset {
    bool USkeleton::SetBones(Core::TArray<Asset::FMeshImportBone> Bones) {
        if (Bones.empty() || Bones.size() > Asset::MaxSkeletonBones) {
            return false;
        }

        Core::TArray<Core::FMatrix> GlobalBindPoses{};
        Core::TArray<Core::FMatrix> InverseBindPoses{};
        Core::TArray<Core::FTransform> ReferencePose(Bones.size());

        GlobalBindPoses.reserve(Bones.size());
        InverseBindPoses.reserve(Bones.size());

        for (std::size_t Index{}; Index < Bones.size(); ++Index) {
            const Asset::FMeshImportBone& Bone{Bones[Index]};
            const Core::Int32 Parent{Bone.GetParentIndex()};

            if (Parent < -1 || Parent >= static_cast<Core::Int32>(Index) || Bone.GetName().find('\0') != Core::FString::npos || !Bone.GetBindPose().IsFinite()) {
                return false;
            }

            const Core::FMatrix Global{Parent < 0 ? Bone.GetBindPose() : Bone.GetBindPose() * GlobalBindPoses[static_cast<std::size_t>(Parent)]};
            Core::FMatrix Inverse{};

            if (!Global.IsFinite() || !Global.TryInverse(Inverse) || !Inverse.IsFinite()) {
                return false;
            }

            GlobalBindPoses.push_back(Global);
            InverseBindPoses.push_back(Inverse);

            // 애니메이션이 트랙 없는 본에 쓰도록 부모 본 기준 바인드 포즈를 위치·회전·스케일로 분해해 둡니다.
            Bone.GetBindPose().TryGetTransform(ReferencePose[Index]);
        }

        mBones = std::move(Bones);
        mGlobalBindPoses = std::move(GlobalBindPoses);
        mInverseBindPoses = std::move(InverseBindPoses);
        mReferencePose = std::move(ReferencePose);
        mRevision = Core::FRevision::Next();

        return true;
    }

    const Core::TArray<Asset::FMeshImportBone>& USkeleton::GetBones() const {
        return mBones;
    }

    const Core::TArray<Core::FMatrix>& USkeleton::GetGlobalBindPoses() const {
        return mGlobalBindPoses;
    }

    const Core::TArray<Core::FMatrix>& USkeleton::GetInverseBindPoses() const {
        return mInverseBindPoses;
    }

    const Core::TArray<Core::FTransform>& USkeleton::GetReferencePose() const {
        return mReferencePose;
    }

    Core::FRevision USkeleton::GetRevision() const {
        return mRevision;
    }

    bool USkeleton::IsValid() const {
        return !mBones.empty();
    }
}
