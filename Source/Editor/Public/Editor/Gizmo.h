#pragma once

#include "Core/ReflectionMacros.h"
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
    ReflectTypes(FTransformGizmo);

public:
    explicit FTransformGizmo(UEditorEngine& Engine);

public:
    ReflectStruct(FTransformGizmo);

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
    ReflectMembers();

private:
    UEditorEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    EGizmoMode mMode{EGizmoMode::Translate};
    ReflectProperty(mMode, "Mode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    EGizmoSpace mSpace{EGizmoSpace::World};
    ReflectProperty(mSpace, "Space", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Int32 mHoveredAxis{-1};
    ReflectProperty(mHoveredAxis, "HoveredAxis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Int32 mActiveAxis{-1};
    ReflectProperty(mActiveAxis, "ActiveAxis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FObjectHandle mTarget{};
    ReflectProperty(mTarget, "Target", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint64 mWorldGeneration{};
    ReflectProperty(mWorldGeneration, "WorldGeneration", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FTransform mStartTransform{};
    ReflectProperty(mStartTransform, "StartTransform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FSceneView mDragView{};
    ReflectProperty(mDragView, "DragView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector2 mStartMouse{};
    ReflectProperty(mStartMouse, "StartMouse", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FVector mAxis{};
    ReflectProperty(mAxis, "Axis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector mPlaneNormal{};
    ReflectProperty(mPlaneNormal, "PlaneNormal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector mStartIntersection{};
    ReflectProperty(mStartIntersection, "StartIntersection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mDragSize{1.0f};
    ReflectProperty(mDragSize, "DragSize", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
