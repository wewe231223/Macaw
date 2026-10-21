#pragma once

#include "CoreUObject/FObjectHandle.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FTransform.h"
#include "Engine/FSceneView.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FViewportInput.h"

class UEditorEngine;

enum class EGizmoMode {
    Translate,
    Rotate,
    Scale
};

enum class EGizmoSpace {
    World,
    Local
};

class FTransformGizmo {
public:
    explicit FTransformGizmo(UEditorEngine& Engine);

public:
    void SetMode(EGizmoMode Mode);
    EGizmoMode GetMode() const;

    void SetSpace(EGizmoSpace Space);
    EGizmoSpace GetSpace() const;

    bool Input(const FViewportInput& Input, const FSceneView& View);
    void Draw(const FSceneView& View, FPrimitiveDrawInterface& DrawInterface);

    void Cancel();
    bool IsDragging() const;
    Int32 GetHoveredAxis() const;

private:
    FVector GetAxis(const FTransform& Transform, Int32 Index) const;
    float GetSize(const FSceneView& View, const FVector& Pivot) const;
    Int32 HitTest(const FVector2& Mouse, const FSceneView& View, const FTransform& Transform) const;

    bool BeginDrag(Int32 Axis, const FVector2& Mouse, const FSceneView& View);
    void UpdateDrag(const FVector2& Mouse);

private:
    UEditorEngine* mEngine{};

    EGizmoMode mMode{EGizmoMode::Translate};
    EGizmoSpace mSpace{EGizmoSpace::World};

    Int32 mHoveredAxis{-1};
    Int32 mActiveAxis{-1};

    FObjectHandle mTarget{};
    Uint64 mWorldGeneration{};

    FTransform mStartTransform{};
    FSceneView mDragView{};
    FVector2 mStartMouse{};

    FVector mAxis{};
    FVector mPlaneNormal{};
    FVector mStartIntersection{};
    float mDragSize{1.0f};
};
