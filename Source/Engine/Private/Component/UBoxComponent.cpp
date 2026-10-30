/**
 * @file UBoxComponent.cpp
 * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UBoxComponent.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    Engine::FCollisionShape Engine::UBoxComponent::GetCollisionShape() const {
        return Engine::FCollisionShape::MakeBox(GetScaledBoxExtent());
    }

    Core::FBox Engine::UBoxComponent::CalcLocalBounds() const {
        return Core::FBox{ -mBoxExtent, mBoxExtent };
    }

    void Engine::UBoxComponent::SetBoxExtent(const Core::FVector& InExtent) {
        mBoxExtent = Core::FVector{ std::max(InExtent.x, 0.0f), std::max(InExtent.y, 0.0f), std::max(InExtent.z, 0.0f) };
    }

    Core::FVector Engine::UBoxComponent::GetScaledBoxExtent() const {
        const Core::FVector Scale{GetTransform().mScale};
        return Core::FVector{mBoxExtent.x * std::abs(Scale.x), mBoxExtent.y * std::abs(Scale.y), mBoxExtent.z * std::abs(Scale.z)};
    }

    const Core::FVector& Engine::UBoxComponent::GetBoxExtent() const {
        return mBoxExtent;
    }

}
