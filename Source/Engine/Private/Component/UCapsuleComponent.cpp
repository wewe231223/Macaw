/**
 * @file UCapsuleComponent.cpp
 * @brief Z축 중심 선분과 반구를 가진 캡슐 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UCapsuleComponent.h"
#include "Core/Archive.h"

#include <algorithm>

FCollisionShape UCapsuleComponent::GetCollisionShape() const {
    return FCollisionShape::MakeCapsule(GetScaledCapsuleRadius(), GetScaledCapsuleHalfHeight());
}

FBox UCapsuleComponent::CalcLocalBounds() const {
    return FBox{ FVector{ -mCapsuleRadius, -mCapsuleRadius, -mCapsuleHalfHeight }, FVector{ mCapsuleRadius, mCapsuleRadius, mCapsuleHalfHeight } };
}

void UCapsuleComponent::Serialize(FArchive& Archive) {
    if (Archive.IsLoading() && !Archive.HasError() && Archive.HasField("CapsuleRadius")) {
        SetCapsuleRadius(0.0f);
    }

    Super::Serialize(Archive);
}

void UCapsuleComponent::SetCapsuleSize(float InRadius, float InHalfHeight) {
    mCapsuleRadius = std::max(0.0f, InRadius);
    mCapsuleHalfHeight = std::max({ 0.0f, InHalfHeight, InRadius });
}

float UCapsuleComponent::GetScaledCapsuleRadius() const {
    return mCapsuleRadius * GetShapeScale();
}

float UCapsuleComponent::GetScaledCapsuleHalfHeight() const {
    return mCapsuleHalfHeight * GetShapeScale();
}

float UCapsuleComponent::GetScaledCapsuleHalfHeightWithoutHemisphere() const {
    return (mCapsuleHalfHeight - mCapsuleRadius) * GetShapeScale();
}

float UCapsuleComponent::GetCapsuleHalfHeight() const {
    return mCapsuleHalfHeight;
}

void UCapsuleComponent::SetCapsuleHalfHeight(float InHalfHeight) {
    mCapsuleHalfHeight = std::max(InHalfHeight, mCapsuleRadius);
}

float UCapsuleComponent::GetCapsuleRadius() const {
    return mCapsuleRadius;
}

void UCapsuleComponent::SetCapsuleRadius(float InRadius) {
    mCapsuleRadius = std::clamp(InRadius, 0.0f, mCapsuleHalfHeight);
}
