#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UPrimitiveComponent.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Archive/FArchive.h"
#include "Core/Debug/ILineDrawContext.h"
#include "World/Component/UMeshComponent.h"

class UCollisionComponent : public UPrimitiveComponent {
public:
    ReflectTypes(UCollisionComponent, UPrimitiveComponent);

public:
    UCollisionComponent();
    ~UCollisionComponent() override = default;

public:
    void OnRegister() override;
    void OnUnregister() override;

    bool IsCollisionEnabled() const;
    void SetCollisionEnabled(bool BEnabled);

    bool Raycast(const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance) const;

    virtual void DrawEditorBounds(ILineDrawContext* LineContext, ELineDepthMode DepthMode) const = 0;

    ReflectClass(UCollisionComponent, EClassFlags::Abstract);

    virtual bool RaycastBounds(const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance) const = 0;

    virtual UMeshComponent* GetMeshComponent() const;

protected:
    void Serialize(FArchive& Archive) override;

private:
    ReflectAccessor(mBCollisionEnabled, IsCollisionEnabled, SetCollisionEnabled, "bCollisionEnabled", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mBCollisionEnabled), ReflectEditor("Collision", "Collision Enabled"));

    bool mBCollisionEnabled{true};

    ReflectMembers();
};
