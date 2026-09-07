#pragma once

#include "UPrimitiveComponent.h"
#include "Core/Base/TypeInfo.h"

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

public:
    JG_DECLARE_DERIVED_TYPEINFO(UCollisionComponent, UPrimitiveComponent);

private:
    FVector3 Extent{ 0.5f, 0.5f, 0.5f };
    bool bCollisionEnabled = true;
};
