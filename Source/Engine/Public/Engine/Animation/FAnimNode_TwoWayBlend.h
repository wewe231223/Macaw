/**
 * @file FAnimNode_TwoWayBlend.h
 * @brief 두 입력 포즈를 Alpha로 섞는 노드입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Animation/FAnimNode_Base.h"

namespace Engine {

    /**
     * @brief 입력 A와 B를 Alpha 비율로 섞습니다. Alpha가 0이면 A, 1이면 B입니다.
     * @details 언리얼 FAnimNode_TwoWayBlend에 대응합니다. 가중치가 거의 0인 입력은 Update와 Evaluate를 건너뛰므로,
     * 그 입력 아래 플레이어의 재생 시간도 멈춥니다.
     */
    struct FAnimNode_TwoWayBlend : public Engine::FAnimNode_Base {
    public:

        /**
         * @brief 이보다 작은 가중치는 0으로 보고 해당 입력을 건너뜁니다.
         */
        static constexpr float RelevantWeightThreshold{0.00001f};

    public:
        void Update(const Engine::FAnimationUpdateContext& Context) override;

        void Evaluate(Engine::FPoseContext& Output) override;

        const Core::FString& GetNodeType() const override;

        void GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs) override;

        void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        /**
         * @brief 노드가 제공하는 값 입력 이름과 연결 주소를 수집합니다.
         * @param OutInputs 값 입력 이름과 비소유 링크 주소를 추가할 출력 배열입니다.
         */
        void GetValueInputs(Core::TArray<Engine::FAnimNodeValueInput>& OutInputs) override;

        /**
         * @brief 마지막 Update에서 쓴 Alpha를 반환합니다.
         * @return 0과 1 사이로 제한한 값입니다.
         */
        float GetBlendAlpha() const;

        /**
         * @brief 최근 갱신에서 계산한 루트 모션 증분을 반환합니다.
         * @return 노드 또는 인스턴스의 이동·회전 증분이며 없으면 항등 변환입니다.
         */
        Core::FTransform GetRootMotionDelta() const override;

    public:

        /**
         * @brief 첫 번째 입력 포즈입니다.
         */
        Engine::FPoseLink mA{};

        /**
         * @brief 두 번째 입력 포즈입니다.
         */
        Engine::FPoseLink mB{};

        /**
         * @brief B의 비율입니다. mAlphaInput이 연결되면 매 Update에서 값 노드의 출력으로 갱신합니다.
         */
        float mAlpha{};

        /**
         * @brief 두 포즈의 블렌드 가중치를 공급하는 Float 입력입니다.
         */
        Engine::FAnimFloatLink mAlphaInput{};

    private:

        /**
         * @brief 마지막 Update에서 0과 1 사이로 제한한 Alpha입니다.
         */
        float mBlendAlpha{};
    };

}
