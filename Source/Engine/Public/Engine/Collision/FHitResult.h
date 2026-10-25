#pragma once

#include "Core/Types.h"
#include "Core/Math/FVector.h"

#include <cfloat>

class UPrimitiveComponent;
class AActor;

struct FHitResult {
public:
    ReflectTypes(FHitResult);

public:
    FHitResult() = default;
    explicit FHitResult(float InTime);

public:
    ReflectStruct(FHitResult);
    void Init();
    void Init(const FVector& Start, const FVector& End);
    AActor* GetActor() const;
    UPrimitiveComponent* GetComponent() const;
    static FHitResult GetReversedHit(const FHitResult& Hit, UPrimitiveComponent* MovedComponent, AActor* MovedActor);

private:
    ReflectMembers();

public:
    Int32 mFaceIndex{-1};
    ReflectProperty(mFaceIndex, "FaceIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mTime{1.0f};
    ReflectProperty(mTime, "Time", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mDistance{FLT_MAX};
    ReflectProperty(mDistance, "Distance", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mLocation{};
    ReflectProperty(mLocation, "Location", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mImpactPoint{};
    ReflectProperty(mImpactPoint, "ImpactPoint", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mNormal{};
    ReflectProperty(mNormal, "Normal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mImpactNormal{};
    ReflectProperty(mImpactNormal, "ImpactNormal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mTraceStart{};
    ReflectProperty(mTraceStart, "TraceStart", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mTraceEnd{};
    ReflectProperty(mTraceEnd, "TraceEnd", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mPenetrationDepth{};
    ReflectProperty(mPenetrationDepth, "PenetrationDepth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mBlockingHit{};
    ReflectProperty(mBlockingHit, "BlockingHit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mStartPenetrating{};
    ReflectProperty(mStartPenetrating, "StartPenetrating", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    UPrimitiveComponent* mHitComponent{};
    ReflectProperty(mHitComponent, "HitComponent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    AActor* mHitActor{};
    ReflectProperty(mHitActor, "HitActor", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
