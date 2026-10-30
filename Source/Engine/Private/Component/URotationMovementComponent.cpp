/**
 * @file URotationMovementComponent.cpp
 * @brief Game과 PIE 월드에서 소유 액터의 루트를 초당 회전 속도에 따라 로컬 또는 월드 공간으로 회전합니다.
 * @ingroup Engine
 */

#include "Engine/Component/URotationMovementComponent.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "Engine/Component/USceneComponent.h"

#include <cmath>

namespace Engine {

    Engine::URotationMovementComponent::URotationMovementComponent() = default;
    Engine::URotationMovementComponent::~URotationMovementComponent() = default;

    void Engine::URotationMovementComponent::Tick(float DeltaTime) {
        Engine::UWorld* World{GetWorld()};
        Engine::AActor* Owner{GetOwner()};

        if (World == nullptr || (World->GetWorldType() != Engine::EWorldType::Game && World->GetWorldType() != Engine::EWorldType::PIE) || !HasBegunPlay() || Owner == nullptr || Owner->IsPendingDestroy() || Owner->GetRootComponent() == nullptr || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
            return;
        }

        Core::FTransform Transform{Owner->GetRootComponent()->GetTransform()};
        const Core::FQuat Delta{(mRotationRate * DeltaTime).Normalized().Quaternion()};
        Transform.mRotation = (mRotationInLocalSpace ? Transform.mRotation * Delta : Delta * Transform.mRotation).Normalized();
        Owner->SetActorTransform(Transform);
    }

    const Core::FRotator& Engine::URotationMovementComponent::GetRotationRate() const {
        return mRotationRate;
    }

    bool Engine::URotationMovementComponent::SetRotationRate(const Core::FRotator& Rate) {
        if (!Rate.IsFinite()) {
            return false;
        }

        mRotationRate = Rate;

        return true;
    }

    bool Engine::URotationMovementComponent::GetRotationInLocalSpace() const {
        return mRotationInLocalSpace;
    }

    bool Engine::URotationMovementComponent::SetRotationInLocalSpace(bool LocalSpace) {
        mRotationInLocalSpace = LocalSpace;

        return true;
    }

}
