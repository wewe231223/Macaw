/**
 * @file ULocomotionAnimInstance.h
 * @brief 속력에 따라 블렌드 스페이스로 Idle/Walk/Run을 섞고, 선택한 시퀀스를 Alpha로 덧섞는 애니메이션 인스턴스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Animation/FAnimNode_BlendSpacePlayer.h"
#include "Engine/Animation/FAnimNode_SequencePlayer.h"
#include "Engine/Animation/FAnimNode_TwoWayBlend.h"
#include "Engine/Animation/UAnimInstance.h"
#include "ULocomotionAnimInstance.generated.h"

namespace Engine {

    /**
     * @brief 노드 그래프로 로코모션을 계산하는 예제 인스턴스입니다.
     * @details 그래프는 다음과 같습니다. 덧섞을 시퀀스를 지정하지 않으면 Locomotion 노드가 바로 루트가 됩니다.
     *
     *     [Blend Space Player: Locomotion] (X ← Speed) ─ A ┐
     *                                                        ├─ [Two-Way Blend] (Alpha ← BlendAlpha) ─ Output Pose
     *     [Sequence Player: Overlay]                    ─ B ┘
     *
     * Speed는 소유 컴포넌트의 수평 속력이며, UseManualSpeed를 켜면 ManualSpeed를 씁니다. 이 엔진에는 캐릭터 이동이 없으므로 수동 값으로 블렌드를 확인할 수 있습니다.
     * 에셋 경로와 수치는 컴포넌트에 저장되어 Details 패널에서 편집합니다.
     */
    UCLASS()
    class ULocomotionAnimInstance final : public Engine::UAnimInstance {
        GENERATED_BODY()

    public:
        ULocomotionAnimInstance();

        ~ULocomotionAnimInstance() override;

    public:
        const Core::FString& GetLocomotionBlendSpace() const;
        bool SetLocomotionBlendSpace(const Core::FString& Path);

        const Core::FString& GetOverlayAnimation() const;
        bool SetOverlayAnimation(const Core::FString& Path);

        float GetBlendAlpha() const;
        void SetBlendAlpha(float Alpha);

        float GetManualSpeed() const;
        void SetManualSpeed(float Speed);

        /**
         * @brief 이번 갱신에서 블렌드 스페이스에 넘긴 속력을 반환합니다.
         * @return 월드 단위/초 속력입니다.
         */
        float GetSpeed() const;

    protected:
        void NativeInitializeAnimation() override;

        void NativeUpdateAnimation(float DeltaSeconds) override;

    private:
        /**
         * @brief Idle/Walk/Run 샘플을 가진 블렌드 스페이스의 Content 기준 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "LocomotionBlendSpace", Getter = GetLocomotionBlendSpace, Setter = SetLocomotionBlendSpace)
        Core::FString mLocomotionBlendSpace{};

        /**
         * @brief Two-Way Blend의 B 입력으로 덧섞을 시퀀스의 Content 기준 경로입니다. 비우면 Two-Way Blend를 쓰지 않습니다.
         */
        UPROPERTY(Edit, Serialize, Name = "OverlayAnimation", Getter = GetOverlayAnimation, Setter = SetOverlayAnimation)
        Core::FString mOverlayAnimation{};

        /**
         * @brief 덧섞을 시퀀스의 비율입니다. 0이면 로코모션만, 1이면 덧섞을 시퀀스만 출력합니다.
         */
        UPROPERTY(Edit, Serialize, Name = "BlendAlpha", Getter = GetBlendAlpha, Setter = SetBlendAlpha)
        float mBlendAlpha{};

        /**
         * @brief 컴포넌트 속력 대신 ManualSpeed를 쓸지 여부입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "UseManualSpeed")
        bool mUseManualSpeed{};

        /**
         * @brief UseManualSpeed를 켰을 때 쓰는 속력입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "ManualSpeed", Getter = GetManualSpeed, Setter = SetManualSpeed)
        float mManualSpeed{};

        /**
         * @brief 이번 갱신의 속력입니다. 블렌드 스페이스 플레이어의 X에 묶여 있으며 저장하지 않습니다.
         */
        UPROPERTY(Edit, ReadOnly, Name = "Speed")
        float mSpeed{};

        /**
         * @brief Idle/Walk/Run을 섞는 블렌드 스페이스 플레이어입니다.
         */
        Engine::FAnimNode_BlendSpacePlayer mLocomotionPlayer{};

        /**
         * @brief 덧섞을 시퀀스를 재생하는 플레이어입니다.
         */
        Engine::FAnimNode_SequencePlayer mOverlayPlayer{};

        /**
         * @brief 로코모션과 덧섞을 시퀀스를 섞는 노드입니다.
         */
        Engine::FAnimNode_TwoWayBlend mOverlayBlend{};
    };

}
