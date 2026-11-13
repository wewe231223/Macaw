/**
 * @file FAnimationRuntime.h
 * @brief 포즈를 섞는 공용 계산입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/Math/FTransform.h"

namespace Engine {

    /**
     * @brief 블렌드 노드가 공유하는 포즈 계산을 모읍니다.
     * @details 언리얼 FAnimationRuntime에 대응합니다.
     */
    class FAnimationRuntime {
    public:

        /**
         * @brief 변환 두 개를 섞습니다. 위치와 스케일은 선형 보간하고 회전은 구면 선형 보간합니다.
         * @param[in] A 첫 번째 변환입니다.
         * @param[in] B 두 번째 변환입니다.
         * @param[in] WeightOfA A의 가중치이며 B의 가중치는 1 - WeightOfA입니다.
         * @return 섞은 변환입니다.
         */
        static Core::FTransform BlendTransforms(const Core::FTransform& A, const Core::FTransform& B, float WeightOfA);

        /**
         * @brief 두 루트 모션 증분을 재생 순서에 맞게 합성합니다.
         * @param A 먼저 적용하거나 비교할 변환입니다.
         * @param B 다음으로 적용하거나 비교할 변환입니다.
         * @return A에 이어 B를 적용하는 합성 변환입니다.
         */
        static Core::FTransform AccumulateRootMotion(const Core::FTransform& A, const Core::FTransform& B);

        /**
         * @brief 컴포넌트 기준의 루트 모션을 액터 변환에 적용합니다.
         * @param RootMotion 컴포넌트 기준의 루트 모션 증분입니다.
         * @param ComponentTransform 컴포넌트의 기준 변환입니다.
         * @param ActorTransform 적용 전 액터 변환입니다.
         * @return 루트 모션 적용 후 액터 변환입니다.
         */
        static Core::FTransform ApplyRootMotionToTransform(const Core::FTransform& RootMotion, const Core::FTransform& ComponentTransform, const Core::FTransform& ActorTransform);

        /**
         * @brief 포즈 두 개를 본마다 섞습니다.
         * @details 두 포즈의 길이가 다르면 짧은 쪽 길이까지만 섞고 나머지는 A를 씁니다. OutPose는 PoseA나 PoseB와 같은 배열이어도 됩니다.
         * @param[in] PoseA 첫 번째 포즈입니다.
         * @param[in] PoseB 두 번째 포즈입니다.
         * @param[in] WeightOfPoseA PoseA의 가중치이며 0과 1 사이로 제한합니다.
         * @param[out] OutPose 결과 포즈이며 PoseA 길이로 맞춥니다.
         */
        static void BlendTwoPosesTogether(const Core::TArray<Core::FTransform>& PoseA, const Core::TArray<Core::FTransform>& PoseB, float WeightOfPoseA, Core::TArray<Core::FTransform>& OutPose);
    };

}
