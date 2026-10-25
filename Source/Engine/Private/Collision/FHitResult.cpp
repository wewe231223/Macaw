#include "Engine/Collision/FHitResult.h"

Int32 FHitResult::GetFaceIndex() const {
    return mFaceIndex;
}

void FHitResult::SetFaceIndex(Int32 Value) {
    mFaceIndex = Value;
}

float FHitResult::GetTime() const {
    return mTime;
}

void FHitResult::SetTime(float Value) {
    mTime = Value;
}

float FHitResult::GetDistance() const {
    return mDistance;
}

void FHitResult::SetDistance(float Value) {
    mDistance = Value;
}

const FVector& FHitResult::GetLocation() const {
    return mLocation;
}

void FHitResult::SetLocation(const FVector& Value) {
    mLocation = Value;
}

const FVector& FHitResult::GetImpactPoint() const {
    return mImpactPoint;
}

void FHitResult::SetImpactPoint(const FVector& Value) {
    mImpactPoint = Value;
}

const FVector& FHitResult::GetNormal() const {
    return mNormal;
}

void FHitResult::SetNormal(const FVector& Value) {
    mNormal = Value;
}

const FVector& FHitResult::GetImpactNormal() const {
    return mImpactNormal;
}

void FHitResult::SetImpactNormal(const FVector& Value) {
    mImpactNormal = Value;
}

const FVector& FHitResult::GetTraceStart() const {
    return mTraceStart;
}

void FHitResult::SetTraceStart(const FVector& Value) {
    mTraceStart = Value;
}

const FVector& FHitResult::GetTraceEnd() const {
    return mTraceEnd;
}

void FHitResult::SetTraceEnd(const FVector& Value) {
    mTraceEnd = Value;
}

float FHitResult::GetPenetrationDepth() const {
    return mPenetrationDepth;
}

void FHitResult::SetPenetrationDepth(float Value) {
    mPenetrationDepth = Value;
}

bool FHitResult::GetBlockingHit() const {
    return mBlockingHit;
}

void FHitResult::SetBlockingHit(bool Value) {
    mBlockingHit = Value;
}

bool FHitResult::GetStartPenetrating() const {
    return mStartPenetrating;
}

void FHitResult::SetStartPenetrating(bool Value) {
    mStartPenetrating = Value;
}

UPrimitiveComponent* FHitResult::GetHitComponent() const {
    return mHitComponent;
}

void FHitResult::SetHitComponent(UPrimitiveComponent* Value) {
    mHitComponent = Value;
}

AActor* FHitResult::GetHitActor() const {
    return mHitActor;
}

void FHitResult::SetHitActor(AActor* Value) {
    mHitActor = Value;
}

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
