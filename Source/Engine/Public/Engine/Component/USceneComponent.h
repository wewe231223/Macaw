/**
 * @file USceneComponent.h
 * @brief 검증된 위치, 회전과 스케일을 보관하는 공간 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "Engine/Component/UActorComponent.h"
#include "Core/Math/FTransform.h"
#include "USceneComponent.generated.h"

namespace Engine {

    UENUM()
    enum class EComponentMobility : Uint8 {
        Static,
        Stationary,
        Movable
    };

    /**
     * @brief 검증된 위치, 회전과 스케일을 보관하는 공간 컴포넌트입니다.
     * @ingroup Engine
     */
    UCLASS()
    class USceneComponent : public Engine::UActorComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief USceneComponent의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        USceneComponent();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~USceneComponent() override;

    public:
        /**
         * @brief 컴포넌트의 위치·회전·스케일을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const FTransform& GetTransform() const;

        /**
         * @brief 유한하고 유효한 변환을 검증하여 공간 컴포넌트에 적용합니다.
         * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool SetTransform(const FTransform& Transform);

        /**
         * @brief 위치, 회전과 스케일이 적용된 월드 행렬을 반환합니다.
         * @return 위치, 회전과 스케일이 적용된 월드 행렬입니다.
         */
        FMatrix GetWorldMatrix() const;

        Engine::EComponentMobility GetMobility() const;
        virtual bool SetMobility(Engine::EComponentMobility Mobility);

    private:
        /**
         * @brief 컴포넌트의 위치·회전·스케일입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetTransform, Setter = SetTransform)
        FTransform mTransform{};

        UPROPERTY(Edit, Serialize, Getter = GetMobility, Setter = SetMobility)
        Engine::EComponentMobility mMobility{Engine::EComponentMobility::Movable};
    };

}
