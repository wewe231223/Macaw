/**
 * @file UBoxComponent.cpp
 * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UBoxComponent.h"

#include <algorithm>
#include <cmath>

FCollisionShape UBoxComponent::GetCollisionShape() const {
    return FCollisionShape::MakeBox(GetScaledBoxExtent());
}

FBox UBoxComponent::CalcLocalBounds() const {
    return FBox{ -mBoxExtent, mBoxExtent };
}

void UBoxComponent::SetBoxExtent(const FVector& InExtent) {
    mBoxExtent = FVector{ std::max(InExtent.x, 0.0f), std::max(InExtent.y, 0.0f), std::max(InExtent.z, 0.0f) };
}

FVector UBoxComponent::GetScaledBoxExtent() const {
    const FVector Scale{GetTransform().mScale};
    return FVector{mBoxExtent.x * std::abs(Scale.x), mBoxExtent.y * std::abs(Scale.y), mBoxExtent.z * std::abs(Scale.z)};
}

const FVector& UBoxComponent::GetBoxExtent() const {
    return mBoxExtent;
}
