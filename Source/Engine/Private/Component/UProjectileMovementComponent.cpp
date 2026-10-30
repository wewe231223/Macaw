/**
 * @file UProjectileMovementComponent.cpp
 * @brief PIE 월드에서만 초기 속도·중력·최대 속도에 따라 소유 액터를 이동하는 발사체 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UProjectileMovementComponent.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "Engine/Component/USceneComponent.h"
#include "Core/Math/FRotator.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    Engine::UProjectileMovementComponent::UProjectileMovementComponent() = default;
    Engine::UProjectileMovementComponent::~UProjectileMovementComponent() = default;

    void Engine::UProjectileMovementComponent::BeginPlay() {
        mVelocity = mInitialVelocity;
        mStopped = false;

        if (mInitialVelocityInLocalSpace && GetOwner() != nullptr && GetOwner()->GetRootComponent() != nullptr) {
            mVelocity = GetOwner()->GetRootComponent()->GetTransform().mRotation.RotateVector(mVelocity);
        }

        if (mMaxSpeed > 0.0f) {
            mVelocity = mVelocity.ClampedLength(mMaxSpeed);
        }
    }

    void Engine::UProjectileMovementComponent::Tick(float DeltaTime) {
        Engine::UWorld* World{GetWorld()};
        Engine::AActor* Owner{GetOwner()};

        if (World == nullptr || World->GetWorldType() != Engine::EWorldType::PIE || !HasBegunPlay() || mStopped || Owner == nullptr || Owner->IsPendingDestroy() || Owner->GetRootComponent() == nullptr || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
            return;
        }

        Core::FTransform Transform{Owner->GetRootComponent()->GetTransform()};
        const Core::FVector Acceleration{0.0f, 0.0f, -9.81f * mGravityScale};
        const float StepTime{DeltaTime / static_cast<float>(std::clamp(std::ceil(DeltaTime / (1.0f / 120.0f)), 1.0f, 128.0f))};
        float Remaining{DeltaTime};

        while (Remaining > 0.0f) {
            const float Step{std::min(StepTime, Remaining)};
            const Core::FVector PreviousVelocity{mVelocity};
            mVelocity += Acceleration * Step;

            if (mMaxSpeed > 0.0f) {
                mVelocity = mVelocity.ClampedLength(mMaxSpeed);
            }

            Transform.mLocation += (PreviousVelocity + mVelocity) * (0.5f * Step);
            Remaining = Step >= Remaining ? 0.0f : Remaining - Step;
        }

        if (mRotationFollowsVelocity && mVelocity.LengthSquared() > 0.000001f) {
            Transform.mRotation = mVelocity.Rotation().Quaternion();
        }

        Owner->SetActorTransform(Transform);
    }

    void Engine::UProjectileMovementComponent::EndPlay() {
        StopMovement();
    }

    void Engine::UProjectileMovementComponent::StopMovement() {
        mVelocity = {};
        mStopped = true;
    }

    const Core::FVector& Engine::UProjectileMovementComponent::GetInitialVelocity() const {
        return mInitialVelocity;
    }

    bool Engine::UProjectileMovementComponent::SetInitialVelocity(const Core::FVector& Velocity) {
        if (!Velocity.IsFinite()) {
            return false;
        }

        mInitialVelocity = Velocity;

        return true;
    }

    const Core::FVector& Engine::UProjectileMovementComponent::GetVelocity() const {
        return mVelocity;
    }

    bool Engine::UProjectileMovementComponent::SetVelocity(const Core::FVector& Velocity) {
        if (!Velocity.IsFinite()) {
            return false;
        }

        mVelocity = mMaxSpeed > 0.0f ? Velocity.ClampedLength(mMaxSpeed) : Velocity;
        mStopped = false;

        return true;
    }

    float Engine::UProjectileMovementComponent::GetMaxSpeed() const {
        return mMaxSpeed;
    }

    bool Engine::UProjectileMovementComponent::SetMaxSpeed(float Speed) {
        if (!std::isfinite(Speed) || Speed < 0.0f) {
            return false;
        }

        mMaxSpeed = Speed;

        if (mMaxSpeed > 0.0f) {
            mVelocity = mVelocity.ClampedLength(mMaxSpeed);
        }

        return true;
    }

    float Engine::UProjectileMovementComponent::GetGravityScale() const {
        return mGravityScale;
    }

    bool Engine::UProjectileMovementComponent::SetGravityScale(float Scale) {
        if (!std::isfinite(Scale) || Scale < 0.0f || Scale > 100.0f) {
            return false;
        }

        mGravityScale = Scale;

        return true;
    }

    bool Engine::UProjectileMovementComponent::GetInitialVelocityInLocalSpace() const {
        return mInitialVelocityInLocalSpace;
    }

    bool Engine::UProjectileMovementComponent::SetInitialVelocityInLocalSpace(bool LocalSpace) {
        mInitialVelocityInLocalSpace = LocalSpace;

        return true;
    }

    bool Engine::UProjectileMovementComponent::GetRotationFollowsVelocity() const {
        return mRotationFollowsVelocity;
    }

    bool Engine::UProjectileMovementComponent::SetRotationFollowsVelocity(bool FollowsVelocity) {
        mRotationFollowsVelocity = FollowsVelocity;

        return true;
    }

}
