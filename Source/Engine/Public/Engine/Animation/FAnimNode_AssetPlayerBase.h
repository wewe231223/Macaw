/**
 * @file FAnimNode_AssetPlayerBase.h
 * @brief 애니메이션 에셋을 시간에 따라 재생하는 노드의 기반입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Animation/FAnimNode_Base.h"

namespace Engine {

    /**
     * @brief 재생 속도·반복 여부와 재생 위치를 관리하는 에셋 플레이어의 기반입니다.
     * @details 언리얼 FAnimNode_AssetPlayerBase에 대응합니다. 재생 속도가 음수이면 역재생하며, 반복하지 않으면 재생 방향의 끝에서 멈추고 IsFinished가 true가 됩니다.
     */
    struct FAnimNode_AssetPlayerBase : public Engine::FAnimNode_Base {
    public:

        /**
         * @brief 재생 위치를 재생 방향의 시작으로 되돌리고 끝남 상태를 지웁니다.
         * @param[in] AnimInstance 그래프를 소유한 인스턴스입니다.
         */
        void Initialize(Engine::UAnimInstance& AnimInstance) override;

        /**
         * @brief 재생 중인 에셋의 길이를 반환합니다.
         * @return 초 단위 길이이며 에셋이 없으면 0입니다.
         */
        virtual double GetCurrentAssetLength() const = 0;

        /**
         * @brief 현재 재생 위치를 반환합니다.
         * @return 초 단위 위치입니다.
         */
        double GetAccumulatedTime() const;

        /**
         * @brief 재생 위치를 설정합니다.
         * @param[in] Time 초 단위 위치이며 [0, 길이]로 제한합니다.
         */
        void SetAccumulatedTime(double Time);

        /**
         * @brief 반복하지 않는 재생이 끝에 도달했는지 반환합니다.
         * @return 끝에서 멈췄으면 true입니다.
         */
        bool IsFinished() const;

        /**
         * @brief 최근 갱신 직전의 에셋 재생 시각을 반환합니다.
         * @return 직전 시각(초)입니다.
         */
        double GetPreviousAssetTime() const;

        /**
         * @brief 최근 갱신에서 진행한 에셋 시간을 반환합니다.
         * @return 진행 시간(초)이며 역재생은 음수입니다.
         */
        double GetAssetTimeDelta() const;

        /**
         * @brief 최근 갱신에서 계산한 루트 모션 증분을 반환합니다.
         * @return 노드 또는 인스턴스의 이동·회전 증분이며 없으면 항등 변환입니다.
         */
        Core::FTransform GetRootMotionDelta() const override;

        /**
         * @brief 재생 위치를 재생 방향의 시작으로 되돌립니다.
         */
        void Restart();

    protected:

        /**
         * @brief 재생 위치를 경과 시간만큼 진행합니다.
         * @details 반복 재생이면 위치를 [0, 길이)로 감고, 아니면 끝에서 멈추고 끝남 상태로 표시합니다.
         * @param[in] DeltaTime 재생 속도를 곱하기 전의 초 단위 경과 시간입니다.
         * @param[in] Length 초 단위 재생 길이입니다.
         */
        void AdvanceTime(float DeltaTime, double Length);

        /**
         * @brief 재생 속도·반복·위치를 시각화 도구에 보일 값으로 추가합니다.
         * @param[out] OutValues 값을 뒤에 추가합니다.
         */
        void GetPlayerDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues);

        /**
         * @brief 에셋 플레이어의 최근 루트 모션 증분을 기록합니다.
         * @param Delta 이동 입력 또는 루트 모션 증분입니다.
         */
        void SetRootMotionDelta(const Core::FTransform& Delta);

    public:

        /**
         * @brief 재생 속도 배율입니다. 음수이면 역재생합니다.
         */
        float mPlayRate{1.0f};

        /**
         * @brief 끝에 도달하면 처음부터 반복할지 여부입니다.
         */
        bool mLoop{true};

    protected:

        /**
         * @brief 초 단위 재생 위치입니다.
         */
        double mInternalTime{};

        /**
         * @brief 반복하지 않는 재생이 끝에 도달했는지 여부입니다.
         */
        bool mFinished{};

    private:

        /**
         * @brief 최근 갱신 직전의 에셋 재생 시각(초)입니다.
         */
        double mPreviousTime{};

        /**
         * @brief 최근 갱신에서 진행한 에셋 시간(초)이며 역재생 시 음수입니다.
         */
        double mAssetTimeDelta{};

        /**
         * @brief 최근 갱신에서 계산한 루트 모션 증분입니다.
         */
        Core::FTransform mRootMotionDelta{};
    };

}
