#pragma once

#include "CoreUObject/TObjectRef.h"
#include "World/AActor.h"
#include "World/Component/UPrimitiveComponent.h"
#include <limits>

struct FWorldRaycastQuery {
public:
    bool Excludes(const UPrimitiveComponent& Component) const;
    bool PrepareRay(const FRay& Ray, FRay& OutRay) const;

public:
    float mMaxDistance{std::numeric_limits<float>::max()};
    TArray<TObjectRef<AActor>> mExcludedActorHierarchies{};
};

struct FWorldRaycastHit {
public:
    TObjectRef<UPrimitiveComponent> mComponent{};
    float mDistance{std::numeric_limits<float>::max()};
    FVector3 mPosition{};
    FVector3 mNormal{};
    Uint32 mFaceIndex{std::numeric_limits<Uint32>::max()};
};

bool RaycastBox(const DirectX::BoundingOrientedBox& Box, const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance);
