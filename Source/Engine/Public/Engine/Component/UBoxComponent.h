#pragma once

#include "Engine/Component/UShapeComponent.h"

class UBoxComponent : public UShapeComponent {
public:
    ReflectTypes(UBoxComponent, UShapeComponent);

public:
    ReflectClass(UBoxComponent);
    FCollisionShape GetCollisionShape() const override;
    FBox CalcLocalBounds() const override;
    void SetBoxExtent(const FVector& InExtent);
    FVector GetScaledBoxExtent() const;

    const FVector& GetBoxExtent() const;

private:
    ReflectMembers();

private:
    FVector mBoxExtent{0.5f, 0.5f, 0.5f};
    ReflectAccessor(mBoxExtent, GetBoxExtent, SetBoxExtent, "BoxExtent", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
