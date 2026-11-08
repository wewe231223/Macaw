/**
 * @file UCameraShakeModifier.h
 * @brief 사인파 진동으로 카메라를 흔드는 기본 카메라 모디파이어입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Core/Math/FVector.h"
#include "Engine/UCameraModifier.h"
#include <random>
#include "UCameraShakeModifier.generated.h"

namespace Engine {

    /**
     * @brief 사인파 진동으로 카메라를 흔드는 기본 카메라 모디파이어입니다.
     * @details 카메라 관리자가 처음부터 하나 갖고 있으며 GetCameraShakeModifier로 얻습니다. 셰이크를 여러 개
     *          동시에 재생할 수 있고 결과는 모두 더합니다. 각 축은 무작위 위상과 조금씩 다른 주파수로 흔들어
     *          축끼리 같은 박자로 움직이지 않게 합니다. 위치 진동은 카메라 기준 축(X 앞, Y 오른쪽, Z 위)으로,
     *          회전 진동은 카메라 기준 Pitch·Yaw·Roll로 더합니다.
     * @ingroup Engine
     */
    UCLASS(NoFactory)
    class UCameraShakeModifier final : public Engine::UCameraModifier {
        GENERATED_BODY()

    public:
        /**
         * @brief 재생 중인 셰이크가 없는 모디파이어를 구성합니다.
         */
        UCameraShakeModifier();

        /**
         * @brief 객체의 수명을 끝냅니다.
         */
        ~UCameraShakeModifier() override;

    public:
        /**
         * @brief 재생 중인 셰이크를 진행하고 시점에 더합니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         * @param[in,out] ViewInfo 흔들 시점입니다.
         * @return 뒤 순서의 모디파이어도 적용하도록 항상 false입니다.
         */
        bool ModifyCamera(float DeltaTime, Engine::FMinimalViewInfo& ViewInfo) override;

        /**
         * @brief 새 셰이크를 재생합니다.
         * @param[in] Duration 재생 시간이며 초 단위입니다. 0 이하이면 StopShake로 멈출 때까지 계속합니다.
         * @param[in] Frequency 초당 진동 횟수입니다.
         * @param[in] LocationAmplitude 카메라 기준 X 앞·Y 오른쪽·Z 위 방향 최대 이동량이며 미터 단위입니다.
         * @param[in] RotationAmplitude X Pitch·Y Yaw·Z Roll 최대 회전량이며 도 단위입니다.
         * @param[in] FieldOfViewAmplitude 시야각 최대 변화량이며 도 단위입니다.
         * @param[in] BlendInTime 처음 세기가 1이 되기까지의 초 단위 시간입니다.
         * @param[in] BlendOutTime 끝날 때 세기가 0이 되기까지의 초 단위 시간입니다.
         * @return 셰이크 식별 번호이며 인자가 유효하지 않으면 0입니다.
         */
        UFUNCTION()
        Core::Int32 StartShake(float Duration, float Frequency, const Core::FVector& LocationAmplitude, const Core::FVector& RotationAmplitude, float FieldOfViewAmplitude, float BlendInTime, float BlendOutTime);

        /**
         * @brief 재생 중인 셰이크를 멈춥니다.
         * @param[in] Id StartShake가 반환한 번호입니다.
         * @param[in] Immediate true이면 바로 멈추고, false이면 BlendOutTime 동안 줄인 뒤 멈춥니다.
         * @return 재생 중인 셰이크를 찾았으면 true입니다.
         */
        UFUNCTION()
        bool StopShake(Core::Int32 Id, bool Immediate);

        /**
         * @brief 재생 중인 셰이크를 모두 멈춥니다.
         * @param[in] Immediate true이면 바로 멈추고, false이면 각자의 BlendOutTime 동안 줄인 뒤 멈춥니다.
         */
        UFUNCTION()
        void StopAllShakes(bool Immediate);

        /**
         * @brief 재생 중인 셰이크 수를 반환합니다.
         * @return 끝나는 중인 셰이크를 포함한 수입니다.
         */
        UFUNCTION()
        Core::Int32 GetActiveShakeCount() const;

    private:
        /**
         * @brief 재생 중인 셰이크 하나의 설정과 진행 상태입니다.
         */
        struct FActiveShake {
            /**
             * @brief StartShake가 반환한 번호입니다.
             */
            Core::Int32 mId{};

            /**
             * @brief 재생 시간이며 0 이하이면 멈출 때까지 계속합니다.
             */
            float mDuration{};

            /**
             * @brief 초당 진동 횟수입니다.
             */
            float mFrequency{};

            /**
             * @brief 카메라 기준 최대 이동량입니다.
             */
            Core::FVector mLocationAmplitude{};

            /**
             * @brief Pitch·Yaw·Roll 최대 회전량입니다.
             */
            Core::FVector mRotationAmplitude{};

            /**
             * @brief 시야각 최대 변화량입니다.
             */
            float mFieldOfViewAmplitude{};

            /**
             * @brief 처음 세기가 1이 되기까지의 시간입니다.
             */
            float mBlendInTime{};

            /**
             * @brief 끝날 때 세기가 0이 되기까지의 시간입니다.
             */
            float mBlendOutTime{};

            /**
             * @brief 시작한 뒤 지난 시간입니다.
             */
            float mElapsed{};

            /**
             * @brief StopShake로 줄이기 시작한 시각이며 음수이면 멈추는 중이 아닙니다.
             */
            float mStopTime{-1.0f};

            /**
             * @brief 위치·회전·시야각 7개 축의 시작 위상이며 라디안입니다.
             */
            float mPhase[7]{};

            /**
             * @brief 7개 축의 주파수 배율입니다.
             */
            float mRate[7]{};
        };

        /**
         * @brief 셰이크의 현재 세기를 계산합니다.
         * @param[in] Shake 계산할 셰이크입니다.
         * @return 0부터 1까지의 세기이며, 끝났으면 음수입니다.
         */
        static float EvaluateWeight(const FActiveShake& Shake);

    private:
        /**
         * @brief 재생 중인 셰이크 목록입니다.
         */
        Core::TArray<FActiveShake> mShakes{};

        /**
         * @brief 다음 StartShake가 반환할 번호입니다.
         */
        Core::Int32 mNextId{1};

        /**
         * @brief 축별 위상과 주파수 배율을 정하는 난수 생성기입니다.
         */
        std::mt19937 mRandom{std::random_device{}()};
    };

}
