/**
 * @file UCameraModifier.h
 * @brief 카메라 관리자가 확정한 시점을 그리기 직전에 고치는 카메라 모디파이어의 기반 클래스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "CoreUObject/UObject.h"
#include "UCameraModifier.generated.h"

namespace Engine {

    class APlayerCameraManager;
    struct FMinimalViewInfo;

    /**
     * @brief 카메라 관리자가 확정한 시점을 그리기 직전에 고치는 카메라 모디파이어의 기반 클래스입니다.
     * @details 카메라 관리자가 소유하며, 뷰 타깃 블렌드가 끝난 시점에 Priority가 작은 것부터 차례로 적용합니다.
     *          ModifyCamera가 true를 반환하면 뒤 순서의 모디파이어는 이번 프레임에 적용하지 않습니다.
     *
     *          Alpha는 켜질 때 AlphaInTime 동안 0에서 1로, DisableModifier로 끌 때 AlphaOutTime 동안 1에서 0으로
     *          바뀝니다. 파생 클래스는 GetAlpha()로 효과의 세기를 조절해야 하며, Alpha가 0이 되면 꺼진 상태가 됩니다.
     * @ingroup Engine
     */
    UCLASS()
    class UCameraModifier : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief 켜진 상태에서 Alpha 0부터 시작하는 모디파이어를 구성합니다.
         */
        UCameraModifier();

        /**
         * @brief 객체의 수명을 끝냅니다.
         */
        ~UCameraModifier() override;

    public:
        /**
         * @brief 카메라 관리자에 추가될 때 호출됩니다.
         * @param[in] CameraManager 이 모디파이어를 소유할 카메라 관리자입니다.
         * @return 추가를 계속하려면 true이며, false이면 카메라 관리자가 추가를 취소합니다.
         */
        virtual bool AddedToCamera(Engine::APlayerCameraManager* CameraManager);

        /**
         * @brief 이번 프레임의 시점을 고칩니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         * @param[in,out] ViewInfo 앞 순서까지 적용된 시점이며, 고친 값을 그대로 둡니다.
         * @return 뒤 순서의 모디파이어를 건너뛰려면 true입니다.
         * @note 꺼진 상태에서는 호출되지 않습니다. 기반 구현은 아무것도 하지 않고 false를 반환합니다.
         */
        virtual bool ModifyCamera(float DeltaTime, Engine::FMinimalViewInfo& ViewInfo);

        /**
         * @brief Alpha를 목표값 쪽으로 진행하고, 끄는 중에 0이 되면 꺼진 상태로 바꿉니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         */
        void UpdateAlpha(float DeltaTime);

        /**
         * @brief 이 모디파이어를 소유한 카메라 관리자를 반환합니다.
         * @return 카메라 관리자의 비소유 주소이며, 추가되기 전이면 nullptr입니다.
         */
        UFUNCTION()
        Engine::APlayerCameraManager* GetCameraOwner() const;

        /**
         * @brief 적용 순서를 반환합니다.
         * @return 작을수록 먼저 적용되는 값입니다.
         */
        UFUNCTION()
        Core::Int32 GetPriority() const;

        /**
         * @brief 적용 순서를 지정합니다.
         * @param[in] Priority 작을수록 먼저 적용되며 0부터 255까지로 고정합니다.
         */
        UFUNCTION()
        void SetPriority(Core::Int32 Priority);

        /**
         * @brief 켜질 때 Alpha가 1이 되기까지의 시간을 반환합니다.
         * @return 초 단위 시간입니다.
         */
        UFUNCTION()
        float GetAlphaInTime() const;

        /**
         * @brief 켜질 때 Alpha가 1이 되기까지의 시간을 지정합니다.
         * @param[in] Time 초 단위 시간이며 0 이하이면 바로 1이 됩니다.
         */
        UFUNCTION()
        void SetAlphaInTime(float Time);

        /**
         * @brief 끌 때 Alpha가 0이 되기까지의 시간을 반환합니다.
         * @return 초 단위 시간입니다.
         */
        UFUNCTION()
        float GetAlphaOutTime() const;

        /**
         * @brief 끌 때 Alpha가 0이 되기까지의 시간을 지정합니다.
         * @param[in] Time 초 단위 시간이며 0 이하이면 바로 0이 됩니다.
         */
        UFUNCTION()
        void SetAlphaOutTime(float Time);

        /**
         * @brief 현재 효과의 세기를 반환합니다.
         * @return 0부터 1까지의 값입니다.
         */
        UFUNCTION()
        float GetAlpha() const;

        /**
         * @brief 꺼진 상태인지 검사합니다.
         * @return 꺼졌으면 true이며, AlphaOutTime 동안 끄는 중이면 false입니다.
         */
        UFUNCTION()
        bool IsDisabled() const;

        /**
         * @brief 모디파이어를 켜고 Alpha를 AlphaInTime 동안 1로 올립니다.
         */
        UFUNCTION()
        void EnableModifier();

        /**
         * @brief 모디파이어를 끕니다.
         * @param[in] Immediate true이면 바로 끄고, false이면 AlphaOutTime 동안 Alpha를 0으로 내린 뒤 끕니다.
         */
        UFUNCTION()
        void DisableModifier(bool Immediate);

    private:
        /**
         * @brief 이 모디파이어를 소유한 카메라 관리자입니다.
         */
        Engine::APlayerCameraManager* mCameraOwner{};

        /**
         * @brief 적용 순서이며 작을수록 먼저 적용합니다.
         */
        Core::Int32 mPriority{127};

        /**
         * @brief 켜질 때 Alpha가 1이 되기까지의 초 단위 시간입니다.
         */
        float mAlphaInTime{};

        /**
         * @brief 끌 때 Alpha가 0이 되기까지의 초 단위 시간입니다.
         */
        float mAlphaOutTime{};

        /**
         * @brief 현재 효과의 세기입니다.
         */
        float mAlpha{};

        /**
         * @brief 꺼진 상태인지 나타냅니다.
         */
        bool mDisabled{};

        /**
         * @brief Alpha를 0으로 내리는 중인지 나타냅니다.
         */
        bool mPendingDisable{};
    };

}
