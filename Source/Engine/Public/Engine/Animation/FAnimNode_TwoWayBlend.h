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

        const char* GetNodeType() const override;

        void GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs) override;

        void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        /**
         * @brief 마지막 Update에서 쓴 Alpha를 반환합니다.
         * @return 0과 1 사이로 제한한 값입니다.
         */
        float GetBlendAlpha() const;

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
         * @brief B의 비율입니다. mAlphaBinding이 있으면 매 Update에서 그 값으로 덮어씁니다.
         */
        float mAlpha{};

        /**
         * @brief Alpha를 읽어 올 인스턴스 변수입니다. 언리얼의 핀 바인딩에 해당합니다.
         */
        const float* mAlphaBinding{};

        /**
         * @brief 시각화 도구에 보일 바인딩 변수 이름입니다.
         */
        Core::FString mAlphaBindingName{};

    private:
        /**
         * @brief 마지막 Update에서 0과 1 사이로 제한한 Alpha입니다.
         */
        float mBlendAlpha{};
    };

}
