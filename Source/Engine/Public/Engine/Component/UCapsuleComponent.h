/**
 * @file UCapsuleComponent.h
 * @brief Z축 중심 선분과 반구를 가진 캡슐 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UShapeComponent.h"
#include "UCapsuleComponent.generated.h"

namespace Engine {

    /**
     * @brief Z축 중심 선분과 반구를 가진 캡슐 충돌 컴포넌트입니다.
     */
    UCLASS()
    class UCapsuleComponent : public Engine::UShapeComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Engine::FCollisionShape GetCollisionShape() const override;

        /**
         * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::FBox CalcLocalBounds() const override;
        void Serialize(Core::FArchive& Archive) override;

        /**
         * @brief 캡슐의 로컬 반지름과 반높이를 함께 설정합니다.
         * @param[in] InRadius 형상의 반지름입니다.
         * @param[in] InHalfHeight 반구를 포함한 캡슐 반높이입니다.
         */
        void SetCapsuleSize(float InRadius, float InHalfHeight);

        /**
         * @brief 월드 스케일을 반영한 캡슐 반지름을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetScaledCapsuleRadius() const;

        /**
         * @brief 월드 스케일을 반영한 캡슐 반높이를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetScaledCapsuleHalfHeight() const;

        /**
         * @brief 반구를 제외한 캡슐 중심 선분 반길이를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetScaledCapsuleHalfHeightWithoutHemisphere() const;

        /**
         * @brief 반구를 포함한 캡슐 반높이를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetCapsuleHalfHeight() const;

        /**
         * @brief 반구를 포함한 캡슐 반높이를 설정합니다.
         * @param[in] InHalfHeight 반구를 포함한 캡슐 반높이입니다.
         */
        void SetCapsuleHalfHeight(float InHalfHeight);

        /**
         * @brief 캡슐의 로컬 반지름을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetCapsuleRadius() const;

        /**
         * @brief 캡슐의 로컬 반지름을 설정합니다.
         * @param[in] InRadius 형상의 반지름입니다.
         */
        void SetCapsuleRadius(float InRadius);

    private:
        // 반구를 포함한 높이
        /**
         * @brief 반구를 포함한 캡슐 반높이입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetCapsuleHalfHeight, Setter = SetCapsuleHalfHeight)
        float mCapsuleHalfHeight{1.0f};

        /**
         * @brief 캡슐의 로컬 반지름입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetCapsuleRadius, Setter = SetCapsuleRadius)
        float mCapsuleRadius{0.5f};
    };

}
