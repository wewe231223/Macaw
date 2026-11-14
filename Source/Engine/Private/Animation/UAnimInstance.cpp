/**
 * @file UAnimInstance.cpp
 * @brief 컴포넌트 하나의 애니메이션 변수와 노드 그래프를 소유하고 포즈를 계산합니다.
 * @ingroup Engine
 */

#include "Core/Types.h"
#include "Engine/Animation/UAnimInstance.h"
#include "Asset/FAssetManager.h"
#include "Asset/USkeleton.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/Animation/UBlendSpace.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/UWorld.h"
#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Animation/FAnimationRuntime.h"
#include "Engine/Component/USkeletalMeshComponent.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Engine {

    UAnimInstance::UAnimInstance() = default;

    UAnimInstance::~UAnimInstance() = default;

    void UAnimInstance::InitializeAnimation(Engine::USkeletalMeshComponent* Owner) {
        mOwner = Owner;
        mTrackToBone.clear();
        mPose.clear();
        mOwnerVelocity = {};
        mHasLastOwnerLocation = false;
        ResetRootMotion();

        mDebugVisits.clear();
        mCurrentDebugVisit = -1;
        NativeInitializeAnimation();

        if (mRootNode != nullptr) {
            mRootNode->Initialize(*this);
        }
    }

    void UAnimInstance::UpdateAnimation(float DeltaSeconds) {
        DeltaSeconds = std::isfinite(DeltaSeconds) ? std::max(DeltaSeconds, 0.0f) : 0.0f;
        mRootMotionDelta = {};

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
            mExtractedRootMotion = {};

            return;
        }

        // 트랙이 없는 본은 스켈레톤의 레퍼런스 포즈를 그대로 씁니다.
        GetReferencePose(mPose);
        NativeEvaluateAnimation(mPose);

        if (mRootMotionMode == Engine::ERootMotionMode::RootMotionFromEverything && mRootNode != nullptr) {
            mRootMotionDelta = mRootNode->GetRootMotionDelta();
            mExtractedRootMotion = Engine::FAnimationRuntime::AccumulateRootMotion(mExtractedRootMotion, mRootMotionDelta);
        } else {
            mExtractedRootMotion = {};
        }
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

    Engine::ERootMotionMode UAnimInstance::GetRootMotionMode() const {
        return mRootMotionMode;
    }

    void UAnimInstance::SetRootMotionMode(Engine::ERootMotionMode Mode) {
        if (Mode >= Engine::ERootMotionMode::NoRootMotionExtraction && Mode <= Engine::ERootMotionMode::RootMotionFromEverything && mRootMotionMode != Mode) {
            mRootMotionMode = Mode;
            ResetRootMotion();
        }
    }

    Core::FTransform UAnimInstance::GetRootMotionDelta() const {
        return mRootMotionDelta;
    }

    Core::FTransform UAnimInstance::ConsumeExtractedRootMotion() {
        return std::exchange(mExtractedRootMotion, Core::FTransform{});
    }

    void UAnimInstance::ResetRootMotion() {
        mRootMotionDelta = {};
        mExtractedRootMotion = {};
    }

    Core::FTransform UAnimInstance::ExtractSequenceRootMotion(const Asset::UAnimSequence& Sequence, double StartTime, double DeltaTime, bool Looping) {
        const Asset::USkeleton* Skeleton{GetSkeleton()};

        if (mRootMotionMode == Engine::ERootMotionMode::NoRootMotionExtraction || !Sequence.IsRootMotionEnabled() || Skeleton == nullptr || DeltaTime == 0.0) {
            return {};
        }

        const FSequenceBoneMapping& Mapping{GetSequenceBoneMapping(Sequence)};

        if (Mapping.mRootTrack < 0 || Mapping.mRootBone < 0) {
            return {};
        }

        const Core::FTransform Delta{Sequence.ExtractRootMotion(Mapping.mRootTrack, StartTime, DeltaTime, Looping)};
        const Core::FTransform Lock{Sequence.GetRootLockTransform(Mapping.mRootTrack, Skeleton->GetReferencePose()[static_cast<Core::Index_t>(Mapping.mRootBone)])};
        const Core::FQuat Rotation{(Lock.mRotation * Delta.mRotation * Lock.mRotation.Inverse()).Normalized()};

        return {Lock.mRotation.RotateVector(Delta.mLocation) + Lock.mLocation - Rotation.RotateVector(Lock.mLocation), Rotation, {1.0f, 1.0f, 1.0f}};
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
        mCurrentDebugVisit = mDebugVisits[static_cast<Core::Index_t>(mCurrentDebugVisit)].mParentIndex;
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

        const FSequenceBoneMapping& Mapping{GetSequenceBoneMapping(Sequence)};
        const Core::TArray<Core::Int32>& TrackToBone{Mapping.mTrackToBone};

        Sequence.GetTrackPose(Time, Looping, mTrackPose);

        for (Core::Index_t Track{}; Track < std::min(mTrackPose.size(), TrackToBone.size()); ++Track) {
            const Core::Int32 Bone{TrackToBone[Track]};

            if (Bone >= 0 && Bone < Pose.size()) {
                Pose[Bone] = mTrackPose[Track];
            }
        }

        if (Mapping.mRootTrack >= 0 && Mapping.mRootBone >= 0 && (Sequence.IsRootLockForced() || (Sequence.IsRootMotionEnabled() && mRootMotionMode != Engine::ERootMotionMode::NoRootMotionExtraction))) {
            const Core::Int32 RootBone{Mapping.mRootBone};
            const Core::FTransform Lock{Sequence.GetRootLockTransform(Mapping.mRootTrack, Skeleton->GetReferencePose()[RootBone])};

            Pose[RootBone].mLocation = Lock.mLocation;
            Pose[RootBone].mRotation = Lock.mRotation;
        }
    }

    const UAnimInstance::FSequenceBoneMapping& UAnimInstance::GetSequenceBoneMapping(const Asset::UAnimSequence& Sequence) {
        const CoreUObject::TObjectRef<const Asset::UAnimSequence> Handle{&Sequence};
        auto Mapping{mTrackToBone.find(Handle)};

        if (Mapping != mTrackToBone.end() && Mapping->second.mRootMotionBone == Sequence.GetRootMotionBone()) {
            return Mapping->second;
        }

        FSequenceBoneMapping Data{};

        Data.mRootMotionBone = Sequence.GetRootMotionBone();

        if (const Asset::USkeleton* Skeleton{GetSkeleton()}) {
            const auto& Bones{Skeleton->GetBones()};
            const auto& Tracks{Sequence.GetDataModel().GetBoneAnimationTracks()};
            Core::TMap<Core::FString, Core::Index_t> BoneIndices{};
            std::size_t Matched{};
            Core::Int32 RootCount{};

            Data.mTrackToBone.assign(Tracks.size(), -1);
            BoneIndices.reserve(Bones.size());

            for (Core::Index_t Index{}; Index < Bones.size(); ++Index) {
                BoneIndices.emplace(Bones[Index].GetName(), Index);

                if (Bones[Index].GetParentIndex() < 0 && (Data.mRootMotionBone.empty() || Bones[Index].GetName() == Data.mRootMotionBone)) {
                    Data.mRootBone = static_cast<Core::Int32>(Index);
                    ++RootCount;
                }
            }

            if (RootCount != 1) {
                Data.mRootBone = -1;
            }

            for (Core::Index_t Track{}; Track < Tracks.size(); ++Track) {
                if (const auto Found{BoneIndices.find(Tracks[Track].mName)}; Found != BoneIndices.end()) {
                    Data.mTrackToBone[Track] = static_cast<Core::Int32>(Found->second);
                    ++Matched;

                    if (Found->second == Data.mRootBone) {
                        Data.mRootTrack = static_cast<Core::Int32>(Track);
                    }
                }
            }

            if (Matched == 0) {
                Core::ErrorHandler::Report("Animation", "No track of animation '" + Sequence.GetName() + "' matches a bone of the skeleton", Core::ErrorHandler::EErrorLevel::Warning);
            }
        }

        return mTrackToBone.insert_or_assign(Handle, std::move(Data)).first->second;
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
