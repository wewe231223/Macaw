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

    /**
     * @brief Game 또는 PIE의 실행 시작 상태를 확인한 뒤 축별 속도로 소유 액터의 루트 회전을 갱신합니다.
     */
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

    /**
     * @brief 축별 초당 회전 속도를 반환합니다.
     */
    const Core::FRotator& Engine::URotationMovementComponent::GetRotationRate() const {
        return mRotationRate;
    }

    /**
     * @brief 유한한 Pitch·Yaw·Roll 회전 속도를 설정합니다.
     */
    bool Engine::URotationMovementComponent::SetRotationRate(const Core::FRotator& Rate) {
        if (!Rate.IsFinite()) {
            return false;
        }

        mRotationRate = Rate;

        return true;
    }

    /**
     * @brief 회전 증분을 로컬 축으로 합성하는지 조회합니다.
     */
    bool Engine::URotationMovementComponent::GetRotationInLocalSpace() const {
        return mRotationInLocalSpace;
    }

    /**
     * @brief 회전 증분을 로컬 축 또는 월드 축으로 합성하도록 설정합니다.
     */
    bool Engine::URotationMovementComponent::SetRotationInLocalSpace(bool LocalSpace) {
        mRotationInLocalSpace = LocalSpace;

        return true;
    }

}
