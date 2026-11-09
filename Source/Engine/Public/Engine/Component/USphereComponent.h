/**
 * @file USphereComponent.h
 * @brief 로컬 반지름으로 정의한 구 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UShapeComponent.h"
#include "USphereComponent.generated.h"

namespace Engine {

    /**
     * @brief 로컬 반지름으로 정의한 구 충돌 컴포넌트입니다.
     */
    UCLASS()
    class USphereComponent : public Engine::UShapeComponent {
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

        /**
         * @brief 구의 로컬 반지름을 설정합니다.
         * @param[in] InRadius 형상의 반지름입니다.
         */
        void SetSphereRadius(float InRadius);

        /**
         * @brief 월드 스케일을 반영한 구 반지름을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetScaledSphereRadius() const;

        /**
         * @brief 구의 로컬 반지름을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetSphereRadius() const;

    private:
        /**
         * @brief 구의 로컬 반지름입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetSphereRadius, Setter = SetSphereRadius)
        float mSphereRadius{0.5f};
    };

}
