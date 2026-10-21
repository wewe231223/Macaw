#pragma once

#include <memory>
#include "Core/Base/FTransform.h"
#include "World/FWorldRaycast.h"

class UMesh;
struct FMeshPickingSource;

struct FMeshPickingProxy {
public:
    FVector3 TransformNormal(const FVector3& Normal, bool Reverse = false) const;
    const UMesh* GetMesh() const;
    void Update(const UMesh* InMesh, const FTransform& Transform);
    bool PrepareRay(const FRay& Ray, FRay& LocalRay, double& DirectionLength) const;
    bool RaycastPrepared(const FRay& LocalRay, double DirectionLength, FWorldRaycastHit& OutHit, float MaxDistance) const;
    bool Raycast(const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance) const;

public:
    std::shared_ptr<const FMeshPickingSource> mSource{};
    DirectX::XMFLOAT3 mInverseScale{}, mPosition{};
    DirectX::XMFLOAT4 mRotation{};
    bool mValid{}, mReverseWinding{};
};
