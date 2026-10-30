/**
 * @file UCapsuleComponent.cpp
 * @brief Z축 중심 선분과 반구를 가진 캡슐 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UCapsuleComponent.h"
#include "Core/Archive.h"

#include <algorithm>

namespace Engine {

    Engine::FCollisionShape Engine::UCapsuleComponent::GetCollisionShape() const {
        return Engine::FCollisionShape::MakeCapsule(GetScaledCapsuleRadius(), GetScaledCapsuleHalfHeight());
    }

    Core::FBox Engine::UCapsuleComponent::CalcLocalBounds() const {
        return Core::FBox{ Core::FVector{ -mCapsuleRadius, -mCapsuleRadius, -mCapsuleHalfHeight }, Core::FVector{ mCapsuleRadius, mCapsuleRadius, mCapsuleHalfHeight } };
    }

    void Engine::UCapsuleComponent::Serialize(Core::FArchive& Archive) {
        if (Archive.IsLoading() && !Archive.HasError() && Archive.HasField("CapsuleRadius")) {
            SetCapsuleRadius(0.0f);
        }

        Super::Serialize(Archive);
    }

    void Engine::UCapsuleComponent::SetCapsuleSize(float InRadius, float InHalfHeight) {
        mCapsuleRadius = std::max(0.0f, InRadius);
        mCapsuleHalfHeight = std::max({ 0.0f, InHalfHeight, InRadius });
    }

    float Engine::UCapsuleComponent::GetScaledCapsuleRadius() const {
        return mCapsuleRadius * GetShapeScale();
    }

    float Engine::UCapsuleComponent::GetScaledCapsuleHalfHeight() const {
        return mCapsuleHalfHeight * GetShapeScale();
    }

    float Engine::UCapsuleComponent::GetScaledCapsuleHalfHeightWithoutHemisphere() const {
        return (mCapsuleHalfHeight - mCapsuleRadius) * GetShapeScale();
    }

    float Engine::UCapsuleComponent::GetCapsuleHalfHeight() const {
        return mCapsuleHalfHeight;
    }

    void Engine::UCapsuleComponent::SetCapsuleHalfHeight(float InHalfHeight) {
        mCapsuleHalfHeight = std::max(InHalfHeight, mCapsuleRadius);
    }

    float Engine::UCapsuleComponent::GetCapsuleRadius() const {
        return mCapsuleRadius;
    }

    void Engine::UCapsuleComponent::SetCapsuleRadius(float InRadius) {
        mCapsuleRadius = std::clamp(InRadius, 0.0f, mCapsuleHalfHeight);
    }

}
