#pragma once

#include "Engine/Component/UShapeComponent.h"

class UCapsuleComponent : public UShapeComponent {
public:
    ReflectTypes(UCapsuleComponent, UShapeComponent);

public:
    ReflectClass(UCapsuleComponent);
    FCollisionShape GetCollisionShape() const override;
    FBox CalcLocalBounds() const override;
    void Serialize(FArchive& Archive) override;
    void SetCapsuleSize(float InRadius, float InHalfHeight);
    float GetScaledCapsuleRadius() const;
    float GetScaledCapsuleHalfHeight() const;
    float GetScaledCapsuleHalfHeightWithoutHemisphere() const;

    float GetCapsuleHalfHeight() const;
    void SetCapsuleHalfHeight(float InHalfHeight);
    float GetCapsuleRadius() const;
    void SetCapsuleRadius(float InRadius);

private:
    ReflectMembers();

private:
    // 반구를 포함한 높이
    float mCapsuleHalfHeight{1.0f};
    ReflectAccessor(mCapsuleHalfHeight, GetCapsuleHalfHeight, SetCapsuleHalfHeight, "CapsuleHalfHeight", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mCapsuleRadius{0.5f};
    ReflectAccessor(mCapsuleRadius, GetCapsuleRadius, SetCapsuleRadius, "CapsuleRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
