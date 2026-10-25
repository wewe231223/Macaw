#pragma once

#include "Engine/Component/UShapeComponent.h"

class UCapsuleComponent : public UShapeComponent {
public:
    ReflectTypes(UCapsuleComponent, UShapeComponent);

public:
    ReflectClass(UCapsuleComponent);
    FCollisionShape GetCollisionShape() const override;
    FBox CalcLocalBounds() const override;
    void PostEditChangeProperty(const FPropertyChangedEvent& Event) override;
    void SetCapsuleSize(float InRadius, float InHalfHeight);
    float GetScaledCapsuleRadius() const;
    float GetScaledCapsuleHalfHeight() const;
    float GetScaledCapsuleHalfHeightWithoutHemisphere() const;

    float GetCapsuleHalfHeight() const;
    float GetCapsuleRadius() const;

private:
    ReflectMembers();

private:
    // 반구를 포함한 높이
    float mCapsuleHalfHeight{1.0f};
    ReflectProperty(mCapsuleHalfHeight, "CapsuleHalfHeight", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mCapsuleRadius{0.5f};
    ReflectProperty(mCapsuleRadius, "CapsuleRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
