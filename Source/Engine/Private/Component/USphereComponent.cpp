/**
 * @file USphereComponent.cpp
 * @brief 로컬 반지름으로 정의한 구 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/USphereComponent.h"

#include <algorithm>

namespace Engine {

    /**
     * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
     */
    Engine::FCollisionShape Engine::USphereComponent::GetCollisionShape() const {
        return Engine::FCollisionShape::MakeSphere(GetScaledSphereRadius());
    }

    /**
     * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
     */
    Core::FBox Engine::USphereComponent::CalcLocalBounds() const {
        return Core::FBox{ Core::FVector{ -mSphereRadius, -mSphereRadius, -mSphereRadius }, Core::FVector{ mSphereRadius, mSphereRadius, mSphereRadius } };
    }

    /**
     * @brief 구의 로컬 반지름을 설정합니다.
     */
    void Engine::USphereComponent::SetSphereRadius(float InRadius) {
        mSphereRadius = std::max(0.0f, InRadius);
    }

    /**
     * @brief 월드 스케일을 반영한 구 반지름을 반환합니다.
     */
    float Engine::USphereComponent::GetScaledSphereRadius() const {
        return mSphereRadius * GetShapeScale();
    }

    /**
     * @brief 구의 로컬 반지름을 반환합니다.
     */
    float Engine::USphereComponent::GetSphereRadius() const {
        return mSphereRadius;
    }

}
