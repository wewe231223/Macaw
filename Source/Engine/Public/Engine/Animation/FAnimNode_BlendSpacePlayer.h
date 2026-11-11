/**
 * @file FAnimNode_BlendSpacePlayer.h
 * @brief 블렌드 스페이스를 입력 좌표로 재생하는 노드입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Animation/FAnimNode_AssetPlayerBase.h"
#include "Asset/Animation/UBlendSpace.h"

namespace Engine {

    /**
     * @brief 입력 X로 블렌드 스페이스의 샘플 가중치를 구하고, 샘플 포즈를 그 가중치로 섞어 출력합니다.
     * @details 언리얼 FAnimNode_BlendSpacePlayer에 대응합니다. 모든 샘플은 정규화 재생 위치(0~1)를 공유하므로,
     * 길이가 다른 Walk와 Run도 발이 같은 위상에서 섞입니다. 정규화 위치는 가중치를 적용한 샘플 길이의 합으로 진행합니다.
     */
    struct FAnimNode_BlendSpacePlayer : public Engine::FAnimNode_AssetPlayerBase {
    public:
        void Initialize(Engine::UAnimInstance& AnimInstance) override;

        void Update(const Engine::FAnimationUpdateContext& Context) override;

        void Evaluate(Engine::FPoseContext& Output) override;

        const Core::FString& GetNodeType() const override;

        void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;
        void GetValueInputs(Core::TArray<Engine::FAnimNodeValueInput>& OutInputs) override;

        /**
         * @brief 마지막 Update에서 쓴 입력에 대한 가중치 합성 길이를 반환합니다.
         * @return 초 단위 길이입니다.
         */
        double GetCurrentAssetLength() const override;

        /**
         * @brief 마지막 Update에서 계산한 샘플 가중치를 반환합니다.
         * @return 가중치가 0보다 큰 샘플입니다.
         */
        const Core::TArray<Asset::FBlendSampleWeight>& GetSampleWeights() const;

        /**
         * @brief 정규화 재생 위치를 반환합니다.
         * @return 0과 1 사이의 값입니다.
         */
        double GetNormalizedTime() const;

    public:
        /**
         * @brief 재생할 블렌드 스페이스입니다. 에셋 관리자가 소유합니다.
         */
        const Asset::UBlendSpace* mBlendSpace{};

        /**
         * @brief X 축 입력입니다. mXInput이 연결되면 매 Update에서 값 노드의 출력으로 갱신합니다.
         */
        float mX{};

        Engine::FAnimFloatLink mXInput{};

    private:
        /**
         * @brief 모든 샘플이 공유하는 0~1 재생 위치입니다.
         */
        double mNormalizedTime{};

        /**
         * @brief 마지막 Update의 가중치 합성 길이입니다.
         */
        double mBlendedLength{};

        /**
         * @brief 마지막 Update에서 계산한 샘플 가중치입니다.
         */
        Core::TArray<Asset::FBlendSampleWeight> mSampleWeights{};
    };

}
