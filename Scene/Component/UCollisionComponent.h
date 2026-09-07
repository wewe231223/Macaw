#pragma once

#include "UPrimitiveComponent.h"
#include "UStaticMeshComponent.h"
#include "Core/Base/TypeInfo.h"
#include "Serialize/FArchive.h"

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
    void MakeRender(FRenderProbe& OutProbe) const override;

    JG_DECLARE_DERIVED_TYPEINFO(UCollisionComponent, UPrimitiveComponent);

    void SetBounds(const FVector3& InCenter, const FVector3& InExtent);

protected:
    void Serialize(FArchive& Archive) override;

private:
    FVector3 LocalCenter{ 0.0f, 0.0f, 0.0f };
    FVector3 Extent{ 0.5f, 0.5f, 0.5f };
    bool bCollisionEnabled = true;

    bool RaycastBounds(const FRay& Ray, float& OutDistance) const;
    bool RaycastMesh(const FRay& Ray, const UStaticMeshComponent& MeshComponent, float& OutDistance) const;
};
