#include "Engine/Component/UCapsuleComponent.h"
#include "Core/Math/FBox.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/Reflection/FProperty.h"

#include <algorithm>
#include <cmath>

FCollisionShape UCapsuleComponent::GetCollisionShape() const {
    return FCollisionShape::MakeCapsule(GetScaledCapsuleRadius(), GetScaledCapsuleHalfHeight());
}

FBox UCapsuleComponent::CalcLocalBounds() const {
    return FBox{ FVector{ -mCapsuleRadius, -mCapsuleRadius, -mCapsuleHalfHeight }, FVector{ mCapsuleRadius, mCapsuleRadius, mCapsuleHalfHeight } };
}

void UCapsuleComponent::PostEditChangeProperty(const FPropertyChangedEvent& Event) {
    Super::PostEditChangeProperty(Event);
    if (Event.mProperty != nullptr && Event.mProperty->GetName() == "CapsuleRadius") {
        mCapsuleRadius = std::clamp(mCapsuleRadius, 0.0f, mCapsuleHalfHeight);
    }
    else if (Event.mProperty != nullptr && Event.mProperty->GetName() == "CapsuleHalfHeight") {
        mCapsuleHalfHeight = std::max(mCapsuleHalfHeight, mCapsuleRadius);
    }
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

float UCapsuleComponent::GetCapsuleRadius() const {
    return mCapsuleRadius;
}
