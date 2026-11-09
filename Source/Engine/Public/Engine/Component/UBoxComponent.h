/**
 * @file UBoxComponent.h
 * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UShapeComponent.h"
#include "UBoxComponent.generated.h"

namespace Engine {

    /**
     * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
     */
    UCLASS()
    class UBoxComponent : public Engine::UShapeComponent {
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
         * @brief 상자의 로컬 축별 반길이를 설정합니다.
         * @param[in] InExtent 상자 각 축의 반길이입니다.
         */
        void SetBoxExtent(const Core::FVector& InExtent);

        /**
         * @brief 월드 스케일을 반영한 상자 축별 반길이를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::FVector GetScaledBoxExtent() const;

        /**
         * @brief 상자의 로컬 축별 반길이를 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FVector& GetBoxExtent() const;

    private:
        /**
         * @brief 상자의 로컬 축별 반길이입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetBoxExtent, Setter = SetBoxExtent)
        Core::FVector mBoxExtent{0.5f, 0.5f, 0.5f};
    };

}
