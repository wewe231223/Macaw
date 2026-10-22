#include "Editor/Gizmo.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/FWorldContext.h"
#include "Core/Math.h"
#include "Engine/AActor.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FRay.h"
#include "Engine/FSceneView.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FViewportInput.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Editor/EditorEngine.h"

#include <algorithm>
#include <cmath>

namespace {
    std::array<FVector2, 65> BuildUnitCircle() {
        std::array<FVector2, 65> Points{};

        for (std::size_t Index{}; Index + 1 < Points.size(); ++Index) {
            Points[Index] = FVector2{1.0f, 0.0f}.Rotated(360.0f * static_cast<float>(Index) / 64.0f);
        }

        Points.back() = Points.front();

        return Points;
    }

    const std::array<FVector2, 65> UnitCircle{BuildUnitCircle()};
    const std::array<FVector, 3> UnitAxes{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    const std::array<FVector, 3> AxisColors{{{1.0f, 0.28f, 0.22f}, {0.35f, 0.95f, 0.45f}, {0.32f, 0.58f, 1.0f}}};

    FVector Perpendicular(const FVector& Axis) {
        return Axis.Cross(std::abs(Axis.z) < 0.8f ? FVector{0.0f, 0.0f, 1.0f} : FVector{0.0f, 1.0f, 0.0f}).Normalized();
    }
}

FTransformGizmo::FTransformGizmo(UEditorEngine& Engine)
	: mEngine{&Engine} {
}

void FTransformGizmo::SetMode(EGizmoMode Mode) {
    if (!IsDragging()) {
        mMode = Mode;

        if (mMode == EGizmoMode::Scale) {
            mSpace = EGizmoSpace::Local;
        }
    }
}

EGizmoMode FTransformGizmo::GetMode() const {
    return mMode;
}

void FTransformGizmo::SetSpace(EGizmoSpace Space) {
    if (!IsDragging() && (mMode != EGizmoMode::Scale || Space == EGizmoSpace::Local)) {
        mSpace = Space;
    }
}

EGizmoSpace FTransformGizmo::GetSpace() const {
    return mSpace;
}

FVector FTransformGizmo::GetAxis(const FTransform& Transform, Int32 Index) const {
    return mSpace == EGizmoSpace::Local ? Transform.TransformVectorNoScale(UnitAxes[Index]) : UnitAxes[Index];
}

float FTransformGizmo::GetSize(const FSceneView& View, const FVector& Pivot) const {
    return std::max(0.05f, Pivot.Distance(View.mLocation) * 2.0f * std::tan(Math::ToRadians(View.mFieldOfView) * 0.5f) * 90.0f / std::max(View.mSize.y, 1.0f));
}

Int32 FTransformGizmo::HitTest(const FVector2& Mouse, const FSceneView& View, const FTransform& Transform) const {
    const float Size{GetSize(View, Transform.mLocation)};
    float Closest{9.0f};
    Int32 Hit{-1};

    for (Int32 AxisIndex{}; AxisIndex < 3; ++AxisIndex) {
        const FVector Axis{GetAxis(Transform, AxisIndex)};

        if (mMode == EGizmoMode::Rotate) {
            const FVector U{Perpendicular(Axis)};
            const FVector V{Axis.Cross(U)};

            for (Int32 Segment{}; Segment < 64; ++Segment) {
                const FVector2& A{UnitCircle[Segment]};
                const FVector2& B{UnitCircle[Segment + 1]};

                FVector2 Start{};
                FVector2 End{};

                if (Math::Project(Transform.mLocation + (U * A.x + V * A.y) * Size, View.mViewProjection, View.mSize, Start) && Math::Project(Transform.mLocation + (U * B.x + V * B.y) * Size, View.mViewProjection, View.mSize, End)) {
                    const float Distance{Math::DistanceToSegment(Mouse, Start, End)};

                    if (Distance < Closest) {
                        Closest = Distance;
                        Hit = AxisIndex;
                    }
                }
            }
        } else {
            FVector2 Start{};
            FVector2 End{};

            if (Math::Project(Transform.mLocation + Axis * Size * 0.18f, View.mViewProjection, View.mSize, Start) && Math::Project(Transform.mLocation + Axis * Size, View.mViewProjection, View.mSize, End)) {
                const float Distance{Math::DistanceToSegment(Mouse, Start, End)};

                if (Distance < Closest) {
                    Closest = Distance;
                    Hit = AxisIndex;
                }
            }
        }
    }

    return Hit;
}

bool FTransformGizmo::BeginDrag(Int32 Axis, const FVector2& Mouse, const FSceneView& View) {
    AActor* Actor{mEngine->GetSelectedActor()};

    if (Actor == nullptr || Actor->GetRootComponent() == nullptr) {
        return false;
    }

    mStartTransform = Actor->GetRootComponent()->GetTransform();
    mAxis = GetAxis(mStartTransform, Axis);

    mDragView = View;
    mDragSize = GetSize(View, mStartTransform.mLocation);
    mStartMouse = Mouse;

    mPlaneNormal = mMode == EGizmoMode::Rotate ? mAxis : View.mForward.ProjectedOnPlane(mAxis).Normalized();

    if (mPlaneNormal.IsNearlyZero(0.01f)) {
        mPlaneNormal = Perpendicular(mAxis);
    }

    FRay Ray{};

    if (!Math::Deproject(Mouse, View.mSize, View.mViewProjection, Ray) || !Math::IntersectPlane(Ray, mStartTransform.mLocation, mPlaneNormal, mStartIntersection)) {
        return false;
    }

    mTarget = Actor->GetHandle();
    mWorldGeneration = mEngine->GetWorldGeneration();
    mActiveAxis = Axis;

    return true;
}

void FTransformGizmo::UpdateDrag(const FVector2& Mouse) {
    if (mEngine->GetWorldGeneration() != mWorldGeneration || mEngine->GetWorldContext() == nullptr) {
        mActiveAxis = -1;
        return;
    }

    AActor* Actor{Cast<AActor>(mEngine->GetWorldContext()->GetRegistry().Resolve(mTarget))};

    if (Actor == nullptr) {
        mActiveAxis = -1;
        return;
    }

    FRay Ray{};
    FVector Intersection{};

    if (!Math::Deproject(Mouse, mDragView.mSize, mDragView.mViewProjection, Ray) || !Math::IntersectPlane(Ray, mStartTransform.mLocation, mPlaneNormal, Intersection)) {
        return;
    }

    FTransform Transform{mStartTransform};

    if (mMode == EGizmoMode::Translate) {
        Transform.mLocation += (Intersection - mStartIntersection).ProjectedOn(mAxis);
    } else if (mMode == EGizmoMode::Scale) {
        const float Factor{std::max(0.01f, 1.0f + (Intersection - mStartIntersection).Dot(mAxis) / mDragSize)};

        if (mActiveAxis == 0) {
            Transform.mScale.x *= Factor;
        } else if (mActiveAxis == 1) {
            Transform.mScale.y *= Factor;
        } else {
            Transform.mScale.z *= Factor;
        }
    } else {
        const FVector Start{(mStartIntersection - mStartTransform.mLocation).Normalized()};
        const FVector Current{(Intersection - mStartTransform.mLocation).Normalized()};
        const float Angle{std::atan2(mAxis.Dot(Start.Cross(Current)), std::clamp(Start.Dot(Current), -1.0f, 1.0f))};

        Transform.mRotation = FQuat{mAxis, Angle} * mStartTransform.mRotation;
        Transform.NormalizeRotation();
    }

    mEngine->ApplyTransform(*Actor, Transform, true);
}

bool FTransformGizmo::Input(const FViewportInput& Input, const FSceneView& View) {
    if (IsDragging()) {
        if (Input.mEscape) {
            Cancel();
        } else if (Input.mLeftReleased || !Input.mLeftDown) {
            mActiveAxis = -1;
        } else {
            UpdateDrag(Input.mMouse);
        }

        return true;
    }

    const AActor* Actor{mEngine->GetSelectedActor()};
    mHoveredAxis = Actor != nullptr && Actor->GetRootComponent() != nullptr && Input.mHovered ? HitTest(Input.mMouse, View, Actor->GetRootComponent()->GetTransform()) : -1;

    return Input.mLeftPressed && mHoveredAxis >= 0 && BeginDrag(mHoveredAxis, Input.mMouse, View);
}

void FTransformGizmo::Draw(const FSceneView& View, FPrimitiveDrawInterface& DrawInterface) {
    const AActor* Actor{mEngine->GetSelectedActor()};

    if (Actor == nullptr || Actor->GetRootComponent() == nullptr) {
        return;
    }

    const FTransform& Transform{Actor->GetRootComponent()->GetTransform()};
    const FVector Pivot{Transform.mLocation};
    const float Size{GetSize(View, Pivot)};

    for (Int32 Index{}; Index < 3; ++Index) {
        const FVector Axis{GetAxis(Transform, Index)};
        const FVector Color{Index == (IsDragging() ? mActiveAxis : mHoveredAxis) ? FVector{1.0f, 0.9f, 0.25f} : AxisColors[Index]};
        const FVector U{Perpendicular(Axis)};
        const FVector V{Axis.Cross(U)};

        if (mMode == EGizmoMode::Rotate) {
            for (Int32 Segment{}; Segment < 64; ++Segment) {
                const FVector2& A{UnitCircle[Segment]};
                const FVector2& B{UnitCircle[Segment + 1]};

                DrawInterface.DrawLine(Pivot + (U * A.x + V * A.y) * Size, Pivot + (U * B.x + V * B.y) * Size, Color);
            }
        } else {
            const FVector End{Pivot + Axis * Size};

            DrawInterface.DrawLine(Pivot, End, Color);

            if (mMode == EGizmoMode::Translate) {
                DrawInterface.DrawLine(End, End - Axis * Size * 0.18f + U * Size * 0.07f, Color);
                DrawInterface.DrawLine(End, End - Axis * Size * 0.18f - U * Size * 0.07f, Color);
                DrawInterface.DrawLine(End, End - Axis * Size * 0.18f + V * Size * 0.07f, Color);
                DrawInterface.DrawLine(End, End - Axis * Size * 0.18f - V * Size * 0.07f, Color);
            } else {
                const float Radius{Size * 0.055f};

                DrawInterface.DrawLine(End + U * Radius + V * Radius, End - U * Radius + V * Radius, Color);
                DrawInterface.DrawLine(End - U * Radius + V * Radius, End - U * Radius - V * Radius, Color);
                DrawInterface.DrawLine(End - U * Radius - V * Radius, End + U * Radius - V * Radius, Color);
                DrawInterface.DrawLine(End + U * Radius - V * Radius, End + U * Radius + V * Radius, Color);
            }
        }
    }
}

void FTransformGizmo::Cancel() {
    if (IsDragging() && mEngine->GetWorldGeneration() == mWorldGeneration && mEngine->GetWorldContext() != nullptr) {
        if (auto* Actor{Cast<AActor>(mEngine->GetWorldContext()->GetRegistry().Resolve(mTarget))}) {
            mEngine->ApplyTransform(*Actor, mStartTransform);
        }
    }

    mActiveAxis = -1;
    mHoveredAxis = -1;
}

bool FTransformGizmo::IsDragging() const {
    return mActiveAxis >= 0;
}

Int32 FTransformGizmo::GetHoveredAxis() const {
    return mHoveredAxis;
}
