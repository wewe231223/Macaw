/**
 * @file FHitResult.cpp
 * @brief 광선 또는 도형 추적의 적중 위치·법선과 비소유 객체 참조입니다.
 * @ingroup Engine
 */

#include "Engine/Collision/FHitResult.h"

namespace Engine {

    Engine::FHitResult::FHitResult(float InTime)
		: mTime{InTime} {
    }

    void Engine::FHitResult::Init() {
        *this = Engine::FHitResult{};
    }

    void Engine::FHitResult::Init(const Core::FVector& Start, const Core::FVector& End) {
        Init();
        mTraceStart = Start;
        mTraceEnd = End;
    }

    Engine::AActor* Engine::FHitResult::GetActor() const {
        return mHitActor;
    }

    Engine::UPrimitiveComponent* Engine::FHitResult::GetComponent() const {
        return mHitComponent;
    }

    Engine::FHitResult Engine::FHitResult::GetReversedHit(const Engine::FHitResult& Hit, Engine::UPrimitiveComponent* MovedComponent, Engine::AActor* MovedActor) {
        Engine::FHitResult Result{Hit};
        Result.mNormal = -Hit.mNormal;
        Result.mImpactNormal = -Hit.mImpactNormal;
        Result.mHitComponent = MovedComponent;
        Result.mHitActor = MovedActor;

        return Result;
    }

}
