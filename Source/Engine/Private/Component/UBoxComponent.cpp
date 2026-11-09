/**
 * @file UBoxComponent.cpp
 * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UBoxComponent.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    /**
     * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
     */
    Engine::FCollisionShape Engine::UBoxComponent::GetCollisionShape() const {
        return Engine::FCollisionShape::MakeBox(GetScaledBoxExtent());
    }

    /**
     * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
     */
    Core::FBox Engine::UBoxComponent::CalcLocalBounds() const {
        return Core::FBox{ -mBoxExtent, mBoxExtent };
    }

    /**
     * @brief 상자의 로컬 축별 반길이를 설정합니다.
     */
    void Engine::UBoxComponent::SetBoxExtent(const Core::FVector& InExtent) {
        mBoxExtent = Core::FVector{ std::max(InExtent.x, 0.0f), std::max(InExtent.y, 0.0f), std::max(InExtent.z, 0.0f) };
    }

    /**
     * @brief 월드 스케일을 반영한 상자 축별 반길이를 반환합니다.
     */
    Core::FVector Engine::UBoxComponent::GetScaledBoxExtent() const {
        const Core::FVector Scale{GetTransform().mScale};

        return Core::FVector{mBoxExtent.x * std::abs(Scale.x), mBoxExtent.y * std::abs(Scale.y), mBoxExtent.z * std::abs(Scale.z)};
    }

    /**
     * @brief 상자의 로컬 축별 반길이를 반환합니다.
     */
    const Core::FVector& Engine::UBoxComponent::GetBoxExtent() const {
        return mBoxExtent;
    }

}
