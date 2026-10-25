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
