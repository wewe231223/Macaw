#include "Engine/Component/USphereComponent.h"
#include "Core/Math/FBox.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/Reflection/FProperty.h"

#include <algorithm>
#include <cmath>

FCollisionShape USphereComponent::GetCollisionShape() const {
    return FCollisionShape::MakeSphere(GetScaledSphereRadius());
}

FBox USphereComponent::CalcLocalBounds() const {
    return FBox{ FVector{ -mSphereRadius, -mSphereRadius, -mSphereRadius }, FVector{ mSphereRadius, mSphereRadius, mSphereRadius } };
}

void USphereComponent::PostEditChangeProperty(const FPropertyChangedEvent& Event) {
    Super::PostEditChangeProperty(Event);
    mSphereRadius = std::max(0.0f, mSphereRadius);
}

void USphereComponent::SetSphereRadius(float InRadius) {
    mSphereRadius = std::max(0.0f, InRadius);
}

float USphereComponent::GetScaledSphereRadius() const {
    return mSphereRadius * GetShapeScale();
}

float USphereComponent::GetSphereRadius() const {
    return mSphereRadius;
}
