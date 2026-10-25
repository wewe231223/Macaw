#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Collision/FCollisionShape.h"
#include "Engine/Collision/FHitResult.h"
#include "Core/TDelegate.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FBox.h"

class FPrimitiveDrawInterface;

class UShapeComponent : public UPrimitiveComponent {
public:
    ReflectTypes(UShapeComponent, UPrimitiveComponent);
    using FOverlapDelegate = TDelegate<void(UPrimitiveComponent*, AActor*, UPrimitiveComponent*)>;
    using FHitDelegate = TDelegate<void(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, const FHitResult&)>;

public:
    UShapeComponent();

public:
    ReflectClass(UShapeComponent, EClassFlags::Abstract);
    FOverlapDelegate& GetOnComponentBeginOverlap();
    FOverlapDelegate& GetOnComponentEndOverlap();
    FHitDelegate& GetOnComponentHit();
    void OnBeginOverlap(UShapeComponent* Other);
    void OnEndOverlap(UShapeComponent* Other);
    void OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit);
    virtual FCollisionShape GetCollisionShape() const = 0;
    virtual FBox CalcLocalBounds() const = 0;
    FBox CalcBounds() const;
    FBox GetWorldBounds() const override;
    EPrimitiveType GetPrimitiveType() const override;
    void DrawDebug(FPrimitiveDrawInterface& DrawInterface, bool Highlight) const;
    bool IsOverlapping(const UShapeComponent& Other) const;
    bool ComputeContact(const UShapeComponent& Other, FCollisionContact& OutContact) const;
    const FVector4& GetShapeColor() const;
    bool SetShapeColor(const FVector4& Color);
    bool ShouldDrawOnlyIfSelected() const;
    void SetDrawOnlyIfSelected(bool DrawOnlyIfSelected);
    float GetShapeScale() const;

private:
    ReflectMembers();
    bool ShouldCreateRenderState() const override;

private:
    FVector4 mShapeColor{223.0f / 255.0f, 149.0f / 255.0f, 157.0f / 255.0f, 1.0f};
    ReflectAccessor(mShapeColor, GetShapeColor, SetShapeColor, "ShapeColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    bool mDrawOnlyIfSelected{};
    ReflectAccessor(mDrawOnlyIfSelected, ShouldDrawOnlyIfSelected, SetDrawOnlyIfSelected, "DrawOnlyIfSelected", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    FOverlapDelegate mOnComponentBeginOverlap{};
    ReflectProperty(mOnComponentBeginOverlap, "OnComponentBeginOverlap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FOverlapDelegate mOnComponentEndOverlap{};
    ReflectProperty(mOnComponentEndOverlap, "OnComponentEndOverlap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FHitDelegate mOnComponentHit{};
    ReflectProperty(mOnComponentHit, "OnComponentHit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
