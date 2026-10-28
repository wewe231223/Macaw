/**
 * @file FHitResult.cpp
 * @brief 광선 또는 도형 추적의 적중 위치·법선과 비소유 객체 참조입니다.
 * @ingroup Engine
 */

#include "Engine/Collision/FHitResult.h"

FHitResult::FHitResult(float InTime)
	: mTime{InTime} {
}

void FHitResult::Init() {
    *this = FHitResult{};
}

void FHitResult::Init(const FVector& Start, const FVector& End) {
    Init();
    mTraceStart = Start;
    mTraceEnd = End;
}

AActor* FHitResult::GetActor() const {
    return mHitActor;
}

UPrimitiveComponent* FHitResult::GetComponent() const {
    return mHitComponent;
}

FHitResult FHitResult::GetReversedHit(const FHitResult& Hit, UPrimitiveComponent* MovedComponent, AActor* MovedActor) {
    FHitResult Result{Hit};
    Result.mNormal = -Hit.mNormal;
    Result.mImpactNormal = -Hit.mImpactNormal;
    Result.mHitComponent = MovedComponent;
    Result.mHitActor = MovedActor;
    return Result;
}
