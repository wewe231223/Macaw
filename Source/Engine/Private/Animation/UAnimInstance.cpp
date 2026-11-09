#include "Engine/Animation/UAnimInstance.h"
#include "Asset/USkeleton.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/Component/USkeletalMeshComponent.h"

#include <algorithm>

namespace Engine {

    UAnimInstance::UAnimInstance() = default;

    UAnimInstance::~UAnimInstance() = default;

    void UAnimInstance::InitializeAnimation(Engine::USkeletalMeshComponent* Owner) {
        mOwner = Owner;
        ResetTrackMapping();
        mPose.clear();

        NativeInitializeAnimation();
    }

    void UAnimInstance::UpdateAnimation(float DeltaSeconds) {
        NativeUpdateAnimation(DeltaSeconds);

        const Asset::USkeleton* Skeleton{GetSkeleton()};

        if (Skeleton == nullptr) {
            mPose.clear();

            return;
        }

        // 트랙이 없는 본은 스켈레톤의 레퍼런스 포즈를 그대로 씁니다.
        mPose = Skeleton->GetReferencePose();
        NativeEvaluateAnimation(mPose);
    }

    const Core::TArray<Core::FTransform>& UAnimInstance::GetPose() const {
        return mPose;
    }

    Engine::USkeletalMeshComponent* UAnimInstance::GetOwningComponent() const {
        return mOwner;
    }

    void UAnimInstance::NativeInitializeAnimation() {
    }

    void UAnimInstance::NativeUpdateAnimation(float) {
    }

    void UAnimInstance::NativeEvaluateAnimation(Core::TArray<Core::FTransform>&) {
    }

    void UAnimInstance::EvaluateSequence(const Asset::UAnimSequence& Sequence, double Time, bool Looping, Core::TArray<Core::FTransform>& Pose) {
        const Asset::USkeleton* Skeleton{GetSkeleton()};

        if (Skeleton == nullptr || !Sequence.IsValid()) {
            return;
        }

        // 표가 비어 있을 때만 트랙 이름을 본 인덱스로 바꾸는 표를 만듭니다. 시퀀스나 스켈레톤이 바뀌면 ResetTrackMapping이 표를 비웁니다.
        if (mTrackToBone.empty()) {
            const auto& Bones{Skeleton->GetBones()};
            const auto& Tracks{Sequence.GetDataModel().GetBoneAnimationTracks()};
            Core::TMap<Core::FString, Core::Int32> BoneIndices{};
            std::size_t Matched{};

            BoneIndices.reserve(Bones.size());

            for (std::size_t Index{}; Index < Bones.size(); ++Index) {
                BoneIndices.emplace(Bones[Index].GetName(), static_cast<Core::Int32>(Index));
            }

            mTrackToBone.assign(Tracks.size(), -1);

            for (std::size_t Track{}; Track < Tracks.size(); ++Track) {
                if (const auto Found{BoneIndices.find(Tracks[Track].mName)}; Found != BoneIndices.end()) {
                    mTrackToBone[Track] = Found->second;
                    ++Matched;
                }
            }

            if (Matched == 0) {
                Core::ErrorHandler::Report("Animation", "No track of animation '" + Sequence.GetName() + "' matches a bone of the skeleton", Core::ErrorHandler::EErrorLevel::Warning);
            }

        }

        Sequence.GetTrackPose(Time, Looping, mTrackPose);

        for (std::size_t Track{}; Track < std::min(mTrackPose.size(), mTrackToBone.size()); ++Track) {
            const Core::Int32 Bone{mTrackToBone[Track]};

            if (Bone >= 0 && static_cast<std::size_t>(Bone) < Pose.size()) {
                Pose[static_cast<std::size_t>(Bone)] = mTrackPose[Track];
            }
        }
    }

    void UAnimInstance::ResetTrackMapping() {
        mTrackToBone.clear();
    }

    const Asset::USkeleton* UAnimInstance::GetSkeleton() const {
        return mOwner != nullptr ? mOwner->GetSkeleton() : nullptr;
    }

}
