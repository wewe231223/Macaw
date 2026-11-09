/**
 * @file UProjectileMovementComponent.cpp
 * @brief Game과 PIE 월드에서 초기 속도·중력·최대 속도에 따라 소유 액터를 이동하는 발사체 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Engine/Component/UProjectileMovementComponent.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "Engine/Component/USceneComponent.h"
#include "Core/Math/FRotator.h"
#include "Core/Literals.h"

#include <algorithm>
#include <cmath>

using namespace Core::Literals;

namespace Engine {

    /**
     * @brief 기본 설정으로 UProjectileMovementComponent 객체를 초기화합니다.
     */
    Engine::UProjectileMovementComponent::UProjectileMovementComponent() = default;

    /**
     * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Engine::UProjectileMovementComponent::~UProjectileMovementComponent() = default;

    /**
     * @brief 초기 속도를 필요 시 월드 공간으로 변환하고 최대 속도를 적용하여 정지 상태를 해제합니다.
     */
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

    /**
     * @brief Game과 PIE에서 중력·최대 속도를 반영하여 하위 시간 간격으로 적분하고 필요 시 속도 방향으로 회전합니다.
     */
    void Engine::UProjectileMovementComponent::Tick(float DeltaTime) {
        Engine::UWorld* World{GetWorld()};
        Engine::AActor* Owner{GetOwner()};

        if (World == nullptr || (World->GetWorldType() != Engine::EWorldType::Game && World->GetWorldType() != Engine::EWorldType::PIE) || !HasBegunPlay() || mStopped || Owner == nullptr || Owner->IsPendingDestroy() || Owner->GetRootComponent() == nullptr || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
            return;
        }

        Core::FTransform Transform{Owner->GetRootComponent()->GetTransform()};
        constexpr Core::FAcceleration StandardGravity{9.81_mps2};
        constexpr Core::FTime MaxStepTime{1.0f / 120_Hz};
        const Core::FVector Acceleration{0.0f, 0.0f, -(StandardGravity / 1_mps2) * mGravityScale};
        const float StepTime{DeltaTime / static_cast<float>(std::clamp(std::ceil(DeltaTime / (MaxStepTime / 1_s)), 1.0f, 128.0f))};
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

    /**
     * @brief 실행 종료 시 속도를 0으로 만들고 발사체 이동을 정지합니다.
     */
    void Engine::UProjectileMovementComponent::EndPlay() {
        StopMovement();
    }

    /**
     * @brief 현재 속도를 0으로 만들고 다음 속도 설정 또는 실행 시작까지 이동을 정지합니다.
     */
    void Engine::UProjectileMovementComponent::StopMovement() {
        mVelocity = {};
        mStopped = true;
    }

    /**
     * @brief 실행 시작에 사용할 초기 속도 설정을 반환합니다.
     */
    const Core::FVector& Engine::UProjectileMovementComponent::GetInitialVelocity() const {
        return mInitialVelocity;
    }

    /**
     * @brief 유한한 초기 속도를 저장하며 현재 실행 중인 속도는 즉시 바꾸지 않습니다.
     */
    bool Engine::UProjectileMovementComponent::SetInitialVelocity(const Core::FVector& Velocity) {
        if (!Velocity.IsFinite()) {
            return false;
        }

        mInitialVelocity = Velocity;

        return true;
    }

    /**
     * @brief 현재 월드 공간 발사체 속도를 조회합니다.
     */
    const Core::FVector& Engine::UProjectileMovementComponent::GetVelocity() const {
        return mVelocity;
    }

    /**
     * @brief 현재 월드 속도를 설정하고 최대 속도로 제한한 뒤 정지 상태를 해제합니다.
     */
    bool Engine::UProjectileMovementComponent::SetVelocity(const Core::FVector& Velocity) {
        if (!Velocity.IsFinite()) {
            return false;
        }

        mVelocity = mMaxSpeed > 0.0f ? Velocity.ClampedLength(mMaxSpeed) : Velocity;
        mStopped = false;

        return true;
    }

    /**
     * @brief 속도 크기의 상한을 반환합니다.
     */
    float Engine::UProjectileMovementComponent::GetMaxSpeed() const {
        return mMaxSpeed;
    }

    /**
     * @brief 0 이상의 유한한 최대 속도를 저장하고 필요한 경우 현재 속도를 제한합니다.
     */
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

    /**
     * @brief -Z 방향 기본 중력 9.81에 곱할 배율을 반환합니다.
     */
    float Engine::UProjectileMovementComponent::GetGravityScale() const {
        return mGravityScale;
    }

    /**
     * @brief 0~100 범위의 유한한 중력 배율을 설정합니다.
     */
    bool Engine::UProjectileMovementComponent::SetGravityScale(float Scale) {
        if (!std::isfinite(Scale) || Scale < 0.0f || Scale > 100.0f) {
            return false;
        }

        mGravityScale = Scale;

        return true;
    }

    /**
     * @brief 초기 속도를 루트의 로컬 회전으로 변환하는지 조회합니다.
     */
    bool Engine::UProjectileMovementComponent::GetInitialVelocityInLocalSpace() const {
        return mInitialVelocityInLocalSpace;
    }

    /**
     * @brief 실행 시작 시 초기 속도를 로컬 또는 월드 공간으로 해석하도록 설정합니다.
     */
    bool Engine::UProjectileMovementComponent::SetInitialVelocityInLocalSpace(bool LocalSpace) {
        mInitialVelocityInLocalSpace = LocalSpace;

        return true;
    }

    /**
     * @brief 루트의 전방을 현재 속도 방향에 맞추는지 조회합니다.
     */
    bool Engine::UProjectileMovementComponent::GetRotationFollowsVelocity() const {
        return mRotationFollowsVelocity;
    }

    /**
     * @brief 이동 후 속도가 충분히 클 때 루트 회전이 속도 방향을 따르도록 설정합니다.
     */
    bool Engine::UProjectileMovementComponent::SetRotationFollowsVelocity(bool FollowsVelocity) {
        mRotationFollowsVelocity = FollowsVelocity;

        return true;
    }

}
