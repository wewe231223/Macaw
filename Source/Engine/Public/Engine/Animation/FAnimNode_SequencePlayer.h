/**
 * @file FAnimNode_SequencePlayer.h
 * @brief 애니메이션 시퀀스 하나를 재생하는 노드입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Animation/FAnimNode_AssetPlayerBase.h"

namespace Asset {
    class UAnimSequence;
}

namespace Engine {

    /**
     * @brief 시퀀스 하나를 재생 위치에서 샘플링해 포즈로 출력합니다.
     * @details 언리얼 FAnimNode_SequencePlayer에 대응합니다. 시퀀스가 없으면 받은 레퍼런스 포즈를 그대로 둡니다.
     */
    struct FAnimNode_SequencePlayer : public Engine::FAnimNode_AssetPlayerBase {
    public:
        void Update(const Engine::FAnimationUpdateContext& Context) override;

        void Evaluate(Engine::FPoseContext& Output) override;

        const Core::FString& GetNodeType() const override;

        void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        double GetCurrentAssetLength() const override;

        /**
         * @brief 시퀀스 시간의 진행을 일시 정지하거나 재개합니다.
         * @param Paused 시퀀스 시간 진행을 멈출지 여부입니다.
         */
        void SetPaused(bool Paused);

    public:

        /**
         * @brief 재생할 시퀀스입니다. 에셋 관리자가 소유합니다.
         */
        const Asset::UAnimSequence* mSequence{};

    private:

        /**
         * @brief 시퀀스 시간 진행을 일시 정지할지 여부입니다.
         */
        bool mPaused{};
    };

}
