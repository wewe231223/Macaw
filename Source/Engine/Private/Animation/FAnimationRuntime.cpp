#include "Engine/Animation/FAnimationRuntime.h"

#include <algorithm>

namespace Engine {

    Core::FTransform FAnimationRuntime::AccumulateRootMotion(const Core::FTransform& A, const Core::FTransform& B) {
        return {A.mLocation + A.mRotation.RotateVector(B.mLocation), (A.mRotation * B.mRotation).Normalized(), {1.0f, 1.0f, 1.0f}};
    }

    Core::FTransform FAnimationRuntime::ApplyRootMotionToTransform(const Core::FTransform& RootMotion, const Core::FTransform& ComponentTransform, const Core::FTransform& ActorTransform) {
        const Core::FQuat Rotation{(ComponentTransform.mRotation * RootMotion.mRotation * ComponentTransform.mRotation.Inverse()).Normalized()};
        const Core::FVector Scaled{RootMotion.mLocation.x * ComponentTransform.mScale.x, RootMotion.mLocation.y * ComponentTransform.mScale.y, RootMotion.mLocation.z * ComponentTransform.mScale.z};

        return {ComponentTransform.mLocation + ComponentTransform.mRotation.RotateVector(Scaled) + Rotation.RotateVector(ActorTransform.mLocation - ComponentTransform.mLocation), (Rotation * ActorTransform.mRotation).Normalized(), ActorTransform.mScale};
    }

    Core::FTransform FAnimationRuntime::BlendTransforms(const Core::FTransform& A, const Core::FTransform& B, float WeightOfA) {
        const float Alpha{1.0f - WeightOfA};

        return {A.mLocation + (B.mLocation - A.mLocation) * Alpha, A.mRotation.Slerp(B.mRotation, Alpha), A.mScale + (B.mScale - A.mScale) * Alpha};
    }

    void FAnimationRuntime::BlendTwoPosesTogether(const Core::TArray<Core::FTransform>& PoseA, const Core::TArray<Core::FTransform>& PoseB, float WeightOfPoseA, Core::TArray<Core::FTransform>& OutPose) {
        const float Weight{std::clamp(WeightOfPoseA, 0.0f, 1.0f)};
        const std::size_t Count{std::min(PoseA.size(), PoseB.size())};

        // 결과가 입력과 같은 배열일 수 있습니다. 본마다 두 입력을 읽은 뒤 쓰므로 겹쳐도 결과가 같습니다.
        if (&OutPose != &PoseA) {
            OutPose.resize(PoseA.size());

            for (std::size_t Bone{Count}; Bone < PoseA.size(); ++Bone) {
                OutPose[Bone] = PoseA[Bone];
            }
        }

        for (std::size_t Bone{}; Bone < Count; ++Bone) {
            OutPose[Bone] = BlendTransforms(PoseA[Bone], PoseB[Bone], Weight);
        }
    }

}
