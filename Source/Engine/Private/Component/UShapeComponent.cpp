#include "Engine/Component/UShapeComponent.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/EPrimitiveType.h"
#include "Core/Math/FBox.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>

namespace {
    bool BoundsOverlap(const FBox& A, const FBox& B) {
        return A.mMinimum.x <= B.mMaximum.x && A.mMaximum.x >= B.mMinimum.x && A.mMinimum.y <= B.mMaximum.y && A.mMaximum.y >= B.mMinimum.y && A.mMinimum.z <= B.mMaximum.z && A.mMaximum.z >= B.mMinimum.z;
    }

    constexpr Int32 CircleSegment{32};

    // 각도는 모두 라디안
    void AddArc(FPrimitiveDrawInterface& DrawInterface, const FVector& C, const FVector& U, const FVector& V, float R, float StartAngleRad, float EndAngleRad, const FVector& Color) {
        const Int32 Segments{std::max(1, static_cast<Int32>(CircleSegment * (EndAngleRad - StartAngleRad) / (2.0f * std::numbers::pi_v<float>)))};
        const float StepRad{(EndAngleRad - StartAngleRad) / Segments};

        FVector Prev{C + U * (std::cos(StartAngleRad) * R) + V * (std::sin(StartAngleRad) * R)};
        for (Int32 Index{1}; Index <= Segments; ++Index) {
            const float AngleRad{StartAngleRad + StepRad * Index};
            const FVector Point{C + U * (std::cos(AngleRad) * R) + V * (std::sin(AngleRad) * R)};
            DrawInterface.DrawLine(Prev, Point, Color, true);
            Prev = Point;
        }
    }
}

UShapeComponent::UShapeComponent() {
    mOnComponentBeginOverlap.Add([](UPrimitiveComponent*, AActor*, UPrimitiveComponent*) {
        std::clog << "BeginOverlap\n";
    });
    mOnComponentEndOverlap.Add([](UPrimitiveComponent*, AActor*, UPrimitiveComponent*) {
        std::clog << "EndOverlap\n";
    });
}

void UShapeComponent::OnBeginOverlap(UShapeComponent* Other) {
    mOnComponentBeginOverlap.Broadcast(this, Other ? Other->GetOwner() : nullptr, Other);
}

void UShapeComponent::OnEndOverlap(UShapeComponent* Other) {
    mOnComponentEndOverlap.Broadcast(this, Other ? Other->GetOwner() : nullptr, Other);
}

void UShapeComponent::OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit) {
    mOnComponentHit.Broadcast(this, OtherActor, OtherComp, Hit);
}

bool UShapeComponent::ShouldCreateRenderState() const {
    return false;
}

void UShapeComponent::DrawDebug(FPrimitiveDrawInterface& DrawInterface, bool Highlight) const {
    FCollisionShape Shape{GetCollisionShape()};
    FVector Pos{GetTransform().mLocation};
    FQuat Rot{GetTransform().mRotation};
    FVector Color{Highlight ? FVector{1.0f, 0.0f, 0.0f} : FVector{mShapeColor.x, mShapeColor.y, mShapeColor.z}};

    if (Shape.IsBox()) {
        FOrientedBox Box{Collision::ToOrientedBox(Shape, Pos, Rot)};

        FVector V0{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
        FVector V1{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
        FVector V2{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
        FVector V3{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
        FVector V4{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
        FVector V5{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
        FVector V6{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
        FVector V7{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};

        DrawInterface.DrawLine(V0, V1, Color, true);
        DrawInterface.DrawLine(V1, V2, Color, true);
        DrawInterface.DrawLine(V2, V3, Color, true);
        DrawInterface.DrawLine(V3, V0, Color, true);
        DrawInterface.DrawLine(V4, V5, Color, true);
        DrawInterface.DrawLine(V5, V6, Color, true);
        DrawInterface.DrawLine(V6, V7, Color, true);
        DrawInterface.DrawLine(V7, V4, Color, true);
        DrawInterface.DrawLine(V0, V4, Color, true);
        DrawInterface.DrawLine(V1, V5, Color, true);
        DrawInterface.DrawLine(V2, V6, Color, true);
        DrawInterface.DrawLine(V3, V7, Color, true);
    }
    else if (Shape.IsSphere()) {
        FCapsuleSegment Sphere{Collision::ToCapsuleSegment(Shape, Pos, Rot)};

        const FVector F{Rot.ForwardVector()};
        const FVector Rt{Rot.RightVector()};
        const FVector Up{Rot.UpVector()};
        const float R{Sphere.GetRadius()};

        AddArc(DrawInterface, Sphere.GetA(), F, Rt, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
        AddArc(DrawInterface, Sphere.GetA(), F, Up, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
        AddArc(DrawInterface, Sphere.GetA(), Rt, Up, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
    }
    else if (Shape.IsCapsule()) {
        FCapsuleSegment Capsule{Collision::ToCapsuleSegment(Shape, Pos, Rot)};

        const FVector F{Rot.ForwardVector()};
        const FVector Rt{Rot.RightVector()};
        const FVector Up{Rot.UpVector()};
        const float R{Capsule.GetRadius()};

        AddArc(DrawInterface, Capsule.GetA(), F, Rt, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);
        AddArc(DrawInterface, Capsule.GetB(), F, Rt, R, 0.0f, 2.0f * std::numbers::pi_v<float>, Color);

        DrawInterface.DrawLine(Capsule.GetA() + F * R, Capsule.GetB() + F * R, Color, true);
        DrawInterface.DrawLine(Capsule.GetA() - F * R, Capsule.GetB() - F * R, Color, true);
        DrawInterface.DrawLine(Capsule.GetA() + Rt * R, Capsule.GetB() + Rt * R, Color, true);
        DrawInterface.DrawLine(Capsule.GetA() - Rt * R, Capsule.GetB() - Rt * R, Color, true);

        AddArc(DrawInterface, Capsule.GetB(), F, Up, R, 0.0f, std::numbers::pi_v<float>, Color);
        AddArc(DrawInterface, Capsule.GetB(), Rt, Up, R, 0.0f, std::numbers::pi_v<float>, Color);
        AddArc(DrawInterface, Capsule.GetA(), F, -Up, R, 0.0f, std::numbers::pi_v<float>, Color);
        AddArc(DrawInterface, Capsule.GetA(), Rt, -Up, R, 0.0f, std::numbers::pi_v<float>, Color);
    }
}

bool UShapeComponent::IsOverlapping(const UShapeComponent& Other) const {
    if (&Other == this || !BoundsOverlap(CalcBounds(), Other.CalcBounds())) {
        return false;
    }

    return Collision::Overlap(GetCollisionShape(), GetTransform().mLocation, GetTransform().mRotation, Other.GetCollisionShape(), Other.GetTransform().mLocation, Other.GetTransform().mRotation);
}

bool UShapeComponent::ComputeContact(const UShapeComponent& Other, FCollisionContact& OutContact) const {
    if (&Other == this || !BoundsOverlap(CalcBounds(), Other.CalcBounds())) {
        return false;
    }

    return Collision::Overlap(GetCollisionShape(), GetTransform().mLocation, GetTransform().mRotation, Other.GetCollisionShape(), Other.GetTransform().mLocation, Other.GetTransform().mRotation, &OutContact);
}

bool UShapeComponent::ShouldDrawOnlyIfSelected() const {
    return mDrawOnlyIfSelected;
}

float UShapeComponent::GetShapeScale() const {
    FVector Scale{GetTransform().mScale};
    return std::min({ std::abs(Scale.x), std::abs(Scale.y), std::abs(Scale.z) });
}

UShapeComponent::FOverlapDelegate& UShapeComponent::GetOnComponentBeginOverlap() {
    return mOnComponentBeginOverlap;
}

UShapeComponent::FOverlapDelegate& UShapeComponent::GetOnComponentEndOverlap() {
    return mOnComponentEndOverlap;
}

UShapeComponent::FHitDelegate& UShapeComponent::GetOnComponentHit() {
    return mOnComponentHit;
}

FBox UShapeComponent::CalcBounds() const {
    return CalcLocalBounds().Transformed(GetWorldMatrix());
}

FBox UShapeComponent::GetWorldBounds() const {
    return CalcBounds();
}

EPrimitiveType UShapeComponent::GetPrimitiveType() const {
    return EPrimitiveType::Cube;
}

const FVector4& UShapeComponent::GetShapeColor() const {
    return mShapeColor;
}

bool UShapeComponent::SetShapeColor(const FVector4& Color) {
    if (!std::isfinite(Color.x) || !std::isfinite(Color.y) || !std::isfinite(Color.z) || !std::isfinite(Color.w)) {
        return false;
    }
    mShapeColor = Color;
    return true;
}

void UShapeComponent::SetDrawOnlyIfSelected(bool DrawOnlyIfSelected) {
    mDrawOnlyIfSelected = DrawOnlyIfSelected;
}
