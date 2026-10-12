#pragma once
#include "World/Component/UPrimitiveComponent.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Archive/FArchive.h"
#include "Core/Debug/ILineDrawContext.h"
#include "World/Component/UMeshComponent.h"
#include "World/Component/UCollisionComponent.generated.h"

UCLASS(Abstract)
class UCollisionComponent : public UPrimitiveComponent {
public:
    GENERATED_TYPES(UCollisionComponent)

public:
    UCollisionComponent();
    ~UCollisionComponent() override = default;

public:
    void OnRegister() override;
    void OnUnregister() override;

    bool IsCollisionEnabled() const;
    void SetCollisionEnabled(bool BEnabled);

    bool Raycast(const FRay& Ray, float& OutDistance) const;
    virtual void DrawEditorBounds(ILineDrawContext* LineContext, ELineDepthMode DepthMode) const = 0;

    GENERATED_BODY(UCollisionComponent)

    virtual bool RaycastBounds(const FRay& Ray, float& OutDistance) const = 0;

    virtual UMeshComponent* GetMeshComponent() const;

protected:
    void Serialize(FArchive& Archive) override;

private:
    UPROPERTY(Serialize, Name = "bCollisionEnabled", Getter = IsCollisionEnabled, Category = "Collision", DisplayName = "Collision Enabled", Edit, Setter = SetCollisionEnabled)
    bool mBCollisionEnabled{true};
};
