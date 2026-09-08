#pragma once

#include "UPrimitiveComponent.h"
#include "Core/Base/TypeInfo.h"
#include "Serialize/FArchive.h"

class UStaticMeshComponent;

class UCollisionComponent : public UPrimitiveComponent
{
public:
    UCollisionComponent() = default;
    ~UCollisionComponent() override = default;

    void OnCreate() override;
    void OnDestroy() override;

    bool IsCollisionEnabled() const;
    void SetCollisionEnabled(bool bEnabled);

    bool Raycast(const FRay& Ray, float& OutDistance) const;

    const FVector3& GetExtent() const;
    void SetExtent(const FVector3& InExtent);
    void MakeRender(FActorProbe& OutProbe) const override;

    JG_DECLARE_DERIVED_TYPEINFO(UCollisionComponent, UPrimitiveComponent);

    void SetBounds(const DirectX::BoundingBox& InBounds);

protected:
    void Serialize(FArchive& Archive) override;

private:
    DirectX::BoundingOrientedBox OBB{}; 
    bool bCollisionEnabled = true;

    bool RaycastBounds(const FRay& Ray, float& OutDistance) const;
    bool RaycastMesh(const FRay& Ray, const UStaticMeshComponent& MeshComponent, float& OutDistance) const;
};
