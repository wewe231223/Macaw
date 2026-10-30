/**
 * @file USphereComponent.cpp
 * @brief 로컬 반지름으로 정의한 구 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/USphereComponent.h"

#include <algorithm>

namespace Engine {

    Engine::FCollisionShape Engine::USphereComponent::GetCollisionShape() const {
        return Engine::FCollisionShape::MakeSphere(GetScaledSphereRadius());
    }

    Core::FBox Engine::USphereComponent::CalcLocalBounds() const {
        return Core::FBox{ Core::FVector{ -mSphereRadius, -mSphereRadius, -mSphereRadius }, Core::FVector{ mSphereRadius, mSphereRadius, mSphereRadius } };
    }

    void Engine::USphereComponent::SetSphereRadius(float InRadius) {
        mSphereRadius = std::max(0.0f, InRadius);
    }

    float Engine::USphereComponent::GetScaledSphereRadius() const {
        return mSphereRadius * GetShapeScale();
    }

    float Engine::USphereComponent::GetSphereRadius() const {
        return mSphereRadius;
    }

}
