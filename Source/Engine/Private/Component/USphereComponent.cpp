/**
 * @file USphereComponent.cpp
 * @brief 로컬 반지름으로 정의한 구 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/USphereComponent.h"

#include <algorithm>

FCollisionShape USphereComponent::GetCollisionShape() const {
    return FCollisionShape::MakeSphere(GetScaledSphereRadius());
}

FBox USphereComponent::CalcLocalBounds() const {
    return FBox{ FVector{ -mSphereRadius, -mSphereRadius, -mSphereRadius }, FVector{ mSphereRadius, mSphereRadius, mSphereRadius } };
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
