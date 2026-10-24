/**
 * @file Gizmo.cpp
 * @brief 이동, 회전과 스케일 기즈모의 화면 질의와 드래그를 구현합니다.
 * @ingroup Editor
 */

#include "Editor/Gizmo.h"
#include "Editor/FEditorViewportClient.h"
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
#include "CoreUObject/FObjectRegistry.h"
#include "Editor/FEditorContext.h"

#include <algorithm>
#include <cmath>

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 기즈모 회전 링에 사용하는 닫힌 단위 원의 65개 점을 생성합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    std::array<FVector2, 65> BuildUnitCircle() {
        std::array<FVector2, 65> Points{};

        for (std::size_t Index{}; Index + 1 < Points.size(); ++Index) {
            Points[Index] = FVector2{1.0f, 0.0f}.Rotated(360.0f * static_cast<float>(Index) / 64.0f);
        }

        Points.back() = Points.front();

        return Points;
    }

    /**
     * @brief 기즈모 회전 링의 캐시된 단위 원 좌표입니다.
     */
    const std::array<FVector2, 65> UnitCircle{BuildUnitCircle()};
    /**
     * @brief 기즈모의 기본 X·Y·Z 단위 축입니다.
     */
    const std::array<FVector, 3> UnitAxes{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    /**
     * @brief 기즈모 X·Y·Z 축의 표시 색상입니다.
     */
    const std::array<FVector, 3> AxisColors{{{1.0f, 0.28f, 0.22f}, {0.35f, 0.95f, 0.45f}, {0.32f, 0.58f, 1.0f}}};

    /**
     * @brief 지정한 축과 수직인 안정적인 단위 벡터를 구합니다.
     * @param[in] Axis 회전 또는 편집에 사용할 축입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Perpendicular(const FVector& Axis) {
        return Axis.Cross(std::abs(Axis.z) < 0.8f ? FVector{0.0f, 0.0f, 1.0f} : FVector{0.0f, 1.0f, 0.0f}).Normalized();
    }
}

/**
 * @brief 에디터 편집 경로와 클라이언트가 제공하는 변환 도구 상태를 연결합니다.
 */
FTransformGizmo::FTransformGizmo(FEditorContext* Context, const FEditorViewportClient* Client)
	: mContext{Context},
	  mClient{Client} {
}

/**
 * @brief 선택한 좌표 공간에 맞는 기즈모 단위 축을 구합니다.
 */
FVector FTransformGizmo::GetAxis(const FTransform& Transform, Int32 Index) const {
    return mClient->GetGizmoSpace() == EGizmoSpace::Local ? Transform.TransformVectorNoScale(UnitAxes[Index]) : UnitAxes[Index];
}

/**
 * @brief 크기를 반환합니다.
 */
float FTransformGizmo::GetSize(const FSceneView& View, const FVector& Pivot) const {
    return std::max(0.05f, Pivot.Distance(View.mLocation) * 2.0f * std::tan(Math::ToRadians(View.mFieldOfView) * 0.5f) * 90.0f / std::max(View.mSize.y, 1.0f));
}

/**
 * @brief 마우스와 투영된 축의 거리로 선택 액터의 기즈모 HitProxy를 구합니다.
 */
FHitProxy FTransformGizmo::HitTest(const FVector2& Mouse, const FSceneView& View) const {
    const AActor* Actor{mContext->GetSelectedActor()};

    if (Actor == nullptr || Actor->GetRootComponent() == nullptr || !mContext->CanEditWorld()) {
        return {};
    }

    const FTransform Transform{Actor->GetRootComponent()->GetTransform()};
    const float Size{GetSize(View, Transform.mLocation)};
    float Closest{9.0f};
    Int32 Hit{-1};

    for (Int32 AxisIndex{}; AxisIndex < 3; ++AxisIndex) {
        const FVector Axis{GetAxis(Transform, AxisIndex)};

        if (mClient->GetGizmoMode() == EGizmoMode::Rotate) {
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

    return Hit >= 0 ? FHitProxy{EHitProxyType::GizmoAxis, Actor->GetHandle(), Hit} : FHitProxy{};
}

/**
 * @brief 선택 축과 대상의 시작 변환을 저장하고 드래그 평면을 구성합니다.
 */
bool FTransformGizmo::BeginDrag(Int32 Axis, const FVector2& Mouse, const FSceneView& View) {
    AActor* Actor{mContext->GetSelectedActor()};

    if (Actor == nullptr || Actor->GetRootComponent() == nullptr || !mContext->CanEditWorld()) {
        return false;
    }

    mStartTransform = Actor->GetRootComponent()->GetTransform();
    mAxis = GetAxis(mStartTransform, Axis);

    mDragView = View;
    mDragSize = GetSize(View, mStartTransform.mLocation);
    mStartMouse = Mouse;

    mPlaneNormal = mClient->GetGizmoMode() == EGizmoMode::Rotate ? mAxis : View.mForward.ProjectedOnPlane(mAxis).Normalized();

    if (mPlaneNormal.IsNearlyZero(0.01f)) {
        mPlaneNormal = Perpendicular(mAxis);
    }

    FRay Ray{};

    if (!Math::Deproject(Mouse, View.mSize, View.mViewProjection, Ray) || !Math::IntersectPlane(Ray, mStartTransform.mLocation, mPlaneNormal, mStartIntersection)) {
        return false;
    }

    mTarget = Actor->GetHandle();
    mWorldHandle = mContext->GetWorldHandle();
    mActiveAxis = Axis;

    return true;
}

/**
 * @brief 마우스 이동량을 활성 모드의 변환으로 바꾸어 선택한 액터에 적용합니다.
 */
void FTransformGizmo::UpdateDrag(const FVector2& Mouse) {
    if (mContext->GetWorldHandle() != mWorldHandle || mContext->GetWorld() == nullptr) {
        mActiveAxis = -1;
        return;
    }

    AActor* Actor{Cast<AActor>(FObjectRegistry::Get()->Resolve(mTarget))};

    if (Actor == nullptr || Actor->GetWorld() != mContext->GetWorld() || Actor->IsPendingDestroy()) {
        mActiveAxis = -1;
        return;
    }

    FRay Ray{};
    FVector Intersection{};

    if (!Math::Deproject(Mouse, mDragView.mSize, mDragView.mViewProjection, Ray) || !Math::IntersectPlane(Ray, mStartTransform.mLocation, mPlaneNormal, Intersection)) {
        return;
    }

    FTransform Transform{mStartTransform};

    if (mClient->GetGizmoMode() == EGizmoMode::Translate) {
        Transform.mLocation += (Intersection - mStartIntersection).ProjectedOn(mAxis);
    } else if (mClient->GetGizmoMode() == EGizmoMode::Scale) {
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

    mContext->ApplyTransform(Actor->GetHandle(), Transform, true);
}

/**
 * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
 */
bool FTransformGizmo::Input(const FViewportInput& Input, const FSceneView& View, const FHitProxy& Hit) {
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

    const AActor* Actor{mContext->GetSelectedActor()};
    mHoveredAxis = Actor != nullptr && Actor->GetRootComponent() != nullptr && mContext->CanEditWorld() && Input.mHovered && Hit.mType == EHitProxyType::GizmoAxis && Hit.mObject == Actor->GetHandle() && Hit.mAxis >= 0 && Hit.mAxis < 3 ? Hit.mAxis : -1;

    return Input.mLeftPressed && mHoveredAxis >= 0 && BeginDrag(mHoveredAxis, Input.mMouse, View);
}

/**
 * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
 */
void FTransformGizmo::Draw(const FSceneView& View, FPrimitiveDrawInterface& DrawInterface) {
    const AActor* Actor{mContext->GetSelectedActor()};

    if (Actor == nullptr || Actor->GetRootComponent() == nullptr || !mContext->CanEditWorld()) {
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

        if (mClient->GetGizmoMode() == EGizmoMode::Rotate) {
            for (Int32 Segment{}; Segment < 64; ++Segment) {
                const FVector2& A{UnitCircle[Segment]};
                const FVector2& B{UnitCircle[Segment + 1]};

                DrawInterface.DrawLine(Pivot + (U * A.x + V * A.y) * Size, Pivot + (U * B.x + V * B.y) * Size, Color);
            }
        } else {
            const FVector End{Pivot + Axis * Size};

            DrawInterface.DrawLine(Pivot, End, Color);

            if (mClient->GetGizmoMode() == EGizmoMode::Translate) {
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

/**
 * @brief 진행 중인 기즈모 드래그를 취소하고 가능한 경우 시작 변환을 복원합니다.
 */
void FTransformGizmo::Cancel() {
    if (IsDragging() && mContext->GetWorldHandle() == mWorldHandle && mContext->GetWorld() != nullptr) {
        if (auto* Actor{Cast<AActor>(FObjectRegistry::Get()->Resolve(mTarget))}; Actor != nullptr && Actor->GetWorld() == mContext->GetWorld() && !Actor->IsPendingDestroy()) {
            mContext->ApplyTransform(Actor->GetHandle(), mStartTransform);
        }
    }

    mActiveAxis = -1;
    mHoveredAxis = -1;
}

/**
 * @brief 기즈모에서 활성 축을 드래그하고 있는지 검사합니다.
 */
bool FTransformGizmo::IsDragging() const {
    return mActiveAxis >= 0;
}

/**
 * @brief 마우스가 가리키는 축 인덱스, 없으면 -1을 반환합니다.
 */
Int32 FTransformGizmo::GetHoveredAxis() const {
    return mHoveredAxis;
}
