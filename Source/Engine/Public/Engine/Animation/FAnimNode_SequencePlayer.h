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

        const char* GetNodeType() const override;

        void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        double GetCurrentAssetLength() const override;

        void SetPaused(bool Paused);

    public:
        /**
         * @brief 재생할 시퀀스입니다. 에셋 관리자가 소유합니다.
         */
        const Asset::UAnimSequence* mSequence{};

    private:
        bool mPaused{};
    };

}
