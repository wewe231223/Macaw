#pragma once

#include "Engine/Component/UShapeComponent.h"

class USphereComponent : public UShapeComponent {
public:
    ReflectTypes(USphereComponent, UShapeComponent);

public:
    ReflectClass(USphereComponent);
    FCollisionShape GetCollisionShape() const override;
    FBox CalcLocalBounds() const override;
    void SetSphereRadius(float InRadius);
    float GetScaledSphereRadius() const;

    float GetSphereRadius() const;

private:
    ReflectMembers();

private:
    float mSphereRadius{0.5f};
    ReflectAccessor(mSphereRadius, GetSphereRadius, SetSphereRadius, "SphereRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
