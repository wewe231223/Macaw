/**
 * @file UAnimSingleNodeInstance.h
 * @brief 애니메이션 시퀀스 하나를 재생하는 애니메이션 인스턴스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Animation/UAnimInstance.h"
#include "UAnimSingleNodeInstance.generated.h"

namespace Engine {

    /**
     * @brief USkeletalMeshComponent::PlayAnimation이 사용하는, 시퀀스 하나의 재생 시간·반복·속도를 관리하는 인스턴스입니다.
     * @details 언리얼 UAnimSingleNodeInstance에 대응합니다. 노드 그래프 없이 NativeUpdateAnimation에서 시간을 진행하고 NativeEvaluateAnimation에서 시퀀스를 샘플링합니다.
     */
    UCLASS()
    class UAnimSingleNodeInstance final : public Engine::UAnimInstance {
        GENERATED_BODY()

    public:
        UAnimSingleNodeInstance();

        ~UAnimSingleNodeInstance() override;

    public:
        /**
         * @brief 재생할 시퀀스를 바꾸고 재생 위치를 처음으로 되돌립니다.
         * @param[in] Sequence 재생할 시퀀스이며 nullptr이면 레퍼런스 포즈를 출력합니다.
         */
        void SetAnimation(const Asset::UAnimSequence* Sequence);

        /**
         * @brief 재생 중인 시퀀스를 반환합니다.
         * @return 지정하지 않았으면 nullptr입니다.
         */
        const Asset::UAnimSequence* GetAnimation() const;

        /**
         * @brief 현재 위치에서 재생을 시작합니다.
         * @param[in] Looping 끝에 도달하면 처음부터 반복할지 여부입니다.
         */
        void Play(bool Looping);

        /**
         * @brief 현재 위치에서 재생을 멈춥니다.
         */
        void Stop();

        /**
         * @brief 재생 중인지 반환합니다.
         * @return 시간이 진행되고 있으면 true입니다.
         */
        bool IsPlaying() const;

        /**
         * @brief 반복 재생 여부를 설정합니다.
         * @param[in] Looping 끝에 도달하면 처음부터 반복할지 여부입니다.
         */
        void SetLooping(bool Looping);

        /**
         * @brief 재생 속도 배율을 설정합니다.
         * @param[in] PlayRate 1이면 원래 속도이며 음수이면 역재생합니다.
         */
        void SetPlayRate(float PlayRate);

        /**
         * @brief 재생 위치를 설정합니다.
         * @param[in] Time 초 단위 재생 위치입니다.
         */
        void SetPosition(double Time);

        /**
         * @brief 현재 재생 위치를 반환합니다.
         * @return 초 단위 재생 위치입니다.
         */
        double GetPosition() const;

    protected:
        void NativeUpdateAnimation(float DeltaSeconds) override;

        void NativeEvaluateAnimation(Core::TArray<Core::FTransform>& Pose) override;

    private:
        /**
         * @brief 재생 중인 시퀀스입니다. 에셋 관리자가 소유합니다.
         */
        const Asset::UAnimSequence* mSequence{};

        /**
         * @brief 초 단위 재생 위치입니다.
         */
        double mPosition{};

        /**
         * @brief 재생 속도 배율입니다.
         */
        float mPlayRate{1.0f};

        /**
         * @brief 반복 재생 여부입니다.
         */
        bool mLooping{true};

        /**
         * @brief 시간이 진행되고 있는지 여부입니다.
         */
        bool mPlaying{};
    };

}
