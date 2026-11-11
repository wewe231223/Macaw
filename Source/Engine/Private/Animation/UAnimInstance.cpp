#include "Engine/Animation/UAnimInstance.h"
#include "Asset/FAssetManager.h"
#include "Asset/USkeleton.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/Animation/UBlendSpace.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/UWorld.h"
#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Component/USkeletalMeshComponent.h"

#include <algorithm>

namespace Engine {

    UAnimInstance::UAnimInstance() = default;

    UAnimInstance::~UAnimInstance() = default;

    void UAnimInstance::InitializeAnimation(Engine::USkeletalMeshComponent* Owner) {
        mOwner = Owner;
        mTrackToBone.clear();
        mPose.clear();
        mOwnerVelocity = {};
        mHasLastOwnerLocation = false;

        mDebugVisits.clear();
        mCurrentDebugVisit = -1;
        NativeInitializeAnimation();

        if (mRootNode != nullptr) {
            mRootNode->Initialize(*this);
        }
    }

    void UAnimInstance::UpdateAnimation(float DeltaSeconds) {
        if (mOwner != nullptr) {
            const Core::FVector Location{mOwner->GetWorldMatrix().Origin()};

            if (mHasLastOwnerLocation && DeltaSeconds > 0.0f) {
                mOwnerVelocity = (Location - mLastOwnerLocation) * (1.0f / DeltaSeconds);
            }

            mLastOwnerLocation = Location;
            mHasLastOwnerLocation = true;
        }

        mDebugVisits.clear();
        mCurrentDebugVisit = -1;
        mUpdatedNodes.clear();
        NativeUpdateAnimation(DeltaSeconds);

        if (mRootNode != nullptr) {
            const Engine::FAnimationUpdateContext Context{*this, DeltaSeconds};
            UpdateNode(*mRootNode, Context);
        }

        if (GetSkeleton() == nullptr) {
            mPose.clear();

            return;
        }

        // 트랙이 없는 본은 스켈레톤의 레퍼런스 포즈를 그대로 씁니다.
        GetReferencePose(mPose);
        NativeEvaluateAnimation(mPose);
    }

    void UAnimInstance::UpdateNode(Engine::FAnimNode_Base& Node, const Engine::FAnimationUpdateContext& Context) {
        if (mUpdatedNodes.emplace(&Node, true).second) {
            Node.Update(Context);
        }
    }

    void UAnimInstance::ResetNodeUpdate(Engine::FAnimNode_Base& Node) {
        mUpdatedNodes.erase(&Node);
    }

    const Core::TArray<Core::FTransform>& UAnimInstance::GetPose() const {
        return mPose;
    }

    Engine::USkeletalMeshComponent* UAnimInstance::GetOwningComponent() const {
        return mOwner;
    }

    Engine::FAnimNode_Base* UAnimInstance::GetRootNode() const {
        return mRootNode;
    }

    const Core::TArray<Engine::FAnimNodeVisit>& UAnimInstance::GetDebugVisits() const {
        return mDebugVisits;
    }

    void UAnimInstance::BeginDebugVisit(const Engine::FAnimNode_Base& Node, const Engine::FPoseLink* Link, float Weight) {
        mDebugVisits.push_back({&Node, Link, Weight, mCurrentDebugVisit});
        mCurrentDebugVisit = static_cast<Core::Int32>(mDebugVisits.size() - 1);
    }

    void UAnimInstance::EndDebugVisit() {
        mCurrentDebugVisit = mDebugVisits[static_cast<std::size_t>(mCurrentDebugVisit)].mParentIndex;
    }

    void UAnimInstance::RecordDebugSample(const Engine::FAnimNode_Base& Node, Core::Int32 SampleIndex, float Weight) {
        if (mCurrentDebugVisit >= 0) {
            mDebugVisits.push_back({&Node, nullptr, Weight, mCurrentDebugVisit, SampleIndex});
        }
    }

    const Core::FVector& UAnimInstance::GetOwnerVelocity() const {
        return mOwnerVelocity;
    }

    void UAnimInstance::GetReferencePose(Core::TArray<Core::FTransform>& OutPose) const {
        const Asset::USkeleton* Skeleton{GetSkeleton()};

        if (Skeleton == nullptr) {
            OutPose.clear();

            return;
        }

        OutPose = Skeleton->GetReferencePose();
    }

    void UAnimInstance::NativeInitializeAnimation() {
    }

    void UAnimInstance::NativeUpdateAnimation(float) {
    }

    void UAnimInstance::NativeEvaluateAnimation(Core::TArray<Core::FTransform>& Pose) {
        if (mRootNode == nullptr) {
            return;
        }

        Engine::FPoseContext Output{*this};

        BeginDebugVisit(*mRootNode, nullptr, 1.0f);
        mRootNode->Evaluate(Output);
        EndDebugVisit();

        // 노드가 스켈레톤과 다른 길이의 포즈를 만들지 않도록 길이가 맞을 때만 받아들입니다.
        if (Output.mPose.size() == Pose.size()) {
            Pose = std::move(Output.mPose);
        }
    }

    void UAnimInstance::EvaluateSequence(const Asset::UAnimSequence& Sequence, double Time, bool Looping, Core::TArray<Core::FTransform>& Pose) {
        const Asset::USkeleton* Skeleton{GetSkeleton()};

        if (Skeleton == nullptr || !Sequence.IsValid()) {
            return;
        }

        auto Mapping{mTrackToBone.find(&Sequence)};

        // 시퀀스를 처음 평가할 때 트랙 이름을 본 인덱스로 바꾸는 표를 만듭니다. 스켈레톤이 바뀌면 InitializeAnimation이 표를 비웁니다.
        if (Mapping == mTrackToBone.end()) {
            const auto& Bones{Skeleton->GetBones()};
            const auto& Tracks{Sequence.GetDataModel().GetBoneAnimationTracks()};
            Core::TMap<Core::FString, Core::Int32> BoneIndices{};
            Core::TArray<Core::Int32> TrackToBone(Tracks.size(), -1);
            std::size_t Matched{};

            BoneIndices.reserve(Bones.size());

            for (std::size_t Index{}; Index < Bones.size(); ++Index) {
                BoneIndices.emplace(Bones[Index].GetName(), static_cast<Core::Int32>(Index));
            }

            for (std::size_t Track{}; Track < Tracks.size(); ++Track) {
                if (const auto Found{BoneIndices.find(Tracks[Track].mName)}; Found != BoneIndices.end()) {
                    TrackToBone[Track] = Found->second;
                    ++Matched;
                }
            }

            if (Matched == 0) {
                Core::ErrorHandler::Report("Animation", "No track of animation '" + Sequence.GetName() + "' matches a bone of the skeleton", Core::ErrorHandler::EErrorLevel::Warning);
            }

            Mapping = mTrackToBone.emplace(&Sequence, std::move(TrackToBone)).first;
        }

        const Core::TArray<Core::Int32>& TrackToBone{Mapping->second};

        Sequence.GetTrackPose(Time, Looping, mTrackPose);

        for (std::size_t Track{}; Track < std::min(mTrackPose.size(), TrackToBone.size()); ++Track) {
            const Core::Int32 Bone{TrackToBone[Track]};

            if (Bone >= 0 && static_cast<std::size_t>(Bone) < Pose.size()) {
                Pose[static_cast<std::size_t>(Bone)] = mTrackPose[Track];
            }
        }
    }

    bool UAnimInstance::GetBoolVariable(const Core::FString&) const {
        return false;
    }

    bool UAnimInstance::HasGraphVariable(const Core::FString&, bool) const {
        return false;
    }

    void UAnimInstance::GetDebugVariables(Core::TArray<Engine::FAnimNodeValue>&) {
    }

    float UAnimInstance::GetFloatVariable(const Core::FString&) const {
        return 0.0f;
    }

    bool UAnimInstance::GetNodeLayout(const Engine::FAnimNode_Base*, float&, float&) const {
        return false;
    }

    void UAnimInstance::PostEditChangeProperty(const CoreUObject::FPropertyChangedEvent& Event) {
        CoreUObject::UObject::PostEditChangeProperty(Event);

        if (mOwner != nullptr) {
            InitializeAnimation(mOwner);
        }
    }

    const Asset::USkeleton* UAnimInstance::GetSkeleton() const {
        return mOwner != nullptr ? mOwner->GetSkeleton() : nullptr;
    }

    const Asset::UAnimSequence* UAnimInstance::LoadAnimSequence(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{mOwner != nullptr && mOwner->GetWorld() != nullptr ? mOwner->GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        const Asset::UAnimSequence* Sequence{Assets->LoadAnimSequence(Core::FilePath::FromUtf8(Path), Error)};

        if (Sequence == nullptr) {
            Core::ErrorHandler::Report("Animation", "Cannot load animation '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Sequence;
    }

    const Asset::UBlendSpace* UAnimInstance::LoadBlendSpace(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{mOwner != nullptr && mOwner->GetWorld() != nullptr ? mOwner->GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        const Asset::UBlendSpace* BlendSpace{Assets->LoadBlendSpace(Core::FilePath::FromUtf8(Path), Error)};

        if (BlendSpace == nullptr) {
            Core::ErrorHandler::Report("Animation", "Cannot load blend space '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return BlendSpace;
    }

}
