#include "Editor/FGizmoCore.h"
#include "Engine/EditorMesh.h"
#include "Engine/MeshPicking.h"
#include "Core/Math.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRay.h"
#include "Core/Literals.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace Core::Literals;

namespace Editor {

    namespace {
        // 기즈모 X·Y·Z 축의 표시 색상입니다.
        const std::array<Core::FVector, 3> AxisColors{{{1.0f, 0.28f, 0.22f}, {0.35f, 0.95f, 0.45f}, {0.32f, 0.58f, 1.0f}}};

        Core::FVector Perpendicular(const Core::FVector& Axis) {
            return Axis.Cross(std::abs(Axis.z) < 0.8f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}).Normalized();
        }

        Core::FMatrix Basis(const Core::FVector& X, const Core::FVector& Y, const Core::FVector& Z, const Core::FVector& Position) {
            Core::FMatrix Result{Core::FMatrix::Identity()};
            const std::array<Core::FVector, 4> Rows{X, Y, Z, Position};

            for (Core::Int32 Index{}; Index < 4; ++Index) {
                Result(Index, 0) = Rows[Index].x;
                Result(Index, 1) = Rows[Index].y;
                Result(Index, 2) = Rows[Index].z;
            }

            return Result;
        }

        float Snap(float Value, float Step, bool Enabled) {
            return Enabled && std::isfinite(Step) && Step > 0.0f ? std::round(Value / Step) * Step : Value;
        }

    }

    const std::array<Core::FVector, 3>& Editor::FGizmoCore::GetUnitAxes() {
        static const std::array<Core::FVector, 3> UnitAxes{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
        return UnitAxes;
    }

    bool Editor::FGizmoCore::Affects(Core::Int32 Handle, Core::Int32 Axis) {
        return Handle == Axis || (Handle == 3 && Axis != 2) || (Handle == 4 && Axis != 1) || (Handle == 5 && Axis != 0);
    }

    float Editor::FGizmoCore::GetSize(const Engine::FSceneView& View, const Core::FVector& Pivot) const {
        const float Depth{(Pivot - View.mLocation).Dot(View.mForward)};
        const float Size{std::isfinite(mSettings.mSize) ? std::clamp(mSettings.mSize, 0.5f, 2.0f) : 1.0f};

        if (View.mOrthographic) {
            return 2.0f / (View.mProjection(1, 1) * std::max(View.mSize.y, 1.0f)) * Size;
        }

        return Depth > 0.05f ? 2.0f * Depth * Core::Math::Tan(View.mFieldOfView * 0.5_deg) / std::max(View.mSize.y, 1.0f) * Size : 0.0f;
    }

    Engine::FHitProxy Editor::FGizmoCore::HitTest(const Core::FVector2& Mouse, const Engine::FSceneView& View, const Editor::FGizmoPlacement& Placement) const {
        Core::FRay Ray{};

        if (!View.Deproject(Mouse, Ray)) {
            return {};
        }

        Engine::FPrimitiveDrawInterface Shapes{};
        BuildHandles(View, Placement, Shapes);

        Engine::FHitProxy Result{};
        Core::Uint32 Priority{std::numeric_limits<Core::Uint32>::max()};
        float Closest{std::numeric_limits<float>::max()};

        for (const Engine::FDrawMesh& Shape : Shapes.GetMeshes()) {
            if (Shape.mHitProxy.mType == Engine::EHitProxyType::None || Shape.mPriority > Priority) {
                continue;
            }

            float Distance{Shape.mPriority == Priority ? Closest : std::numeric_limits<float>::max()};

            if (Engine::RaycastMesh(&Engine::GetEditorMesh(Shape.mMesh), Shape.mWorld, Ray, Distance)) {
                Result = Shape.mHitProxy;
                Closest = Distance;
                Priority = Shape.mPriority;
            }
        }

        return Result;
    }

    bool Editor::FGizmoCore::BeginDrag(Core::Int32 Axis, const Core::FVector2& Mouse, const Engine::FSceneView& View, const Editor::FGizmoPlacement& Placement) {
        if (Axis < 0 || Axis > 7) {
            return false;
        }

        mStartPivot = Placement.mPivot;
        mStartMouse = Mouse;
        mDragView = View;
        mDragSize = GetSize(View, mStartPivot);
        mDragMode = Placement.mMode;
        mDragAxes = Placement.mAxes;
        mTranslation = {};
        mRawAngle = 0.0f;
        mAppliedAngle = 0.0f;
        mScaleDelta = 0.0f;

        mAxis = Axis < 3 ? mDragAxes[Axis] : View.mForward;
        mPlaneNormal = View.mForward;

        if (Axis >= 3 && Axis <= 5) {
            mPlaneNormal = Axis == 3 ? mDragAxes[0].Cross(mDragAxes[1]) : Axis == 4 ? mDragAxes[0].Cross(mDragAxes[2]) : mDragAxes[1].Cross(mDragAxes[2]);
            mAxis = {};

            for (Core::Int32 Index{}; Index < 3; ++Index) {
                if (Affects(Axis, Index)) {
                    mAxis += mDragAxes[Index];
                }
            }
        } else if (mDragMode == Editor::EGizmoMode::Rotate) {
            mPlaneNormal = mAxis;
        } else if (Axis < 3) {
            mPlaneNormal = View.mForward.ProjectedOnPlane(mAxis);
        }

        if (!mPlaneNormal.Normalize()) {
            return false;
        }

        Core::FVector2 Start{};
        Core::FVector2 End{};

        if (!Core::Math::Project(mStartPivot, View.mViewProjection, View.mSize, Start) || !Core::Math::Project(mStartPivot + mAxis * mDragSize * 100.0f, View.mViewProjection, View.mSize, End)) {
            return false;
        }

        mScreenAxis = End - Start;

        const float Length{std::sqrt(mScreenAxis.x * mScreenAxis.x + mScreenAxis.y * mScreenAxis.y)};
        mScreenAxis = Axis >= 6 ? Core::FVector2{0.70710678f, -0.70710678f} : Length > 0.00001f ? mScreenAxis / Length : Core::FVector2{};

        Core::FRay Ray{};

        if (!View.Deproject(Mouse, Ray) || mDragSize <= 0.0f) {
            return false;
        }

        mScreenFallback = std::abs(Ray.mDirection.Dot(mPlaneNormal)) < 0.12f || !Core::Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, mStartIntersection);

        if (mDragMode == Editor::EGizmoMode::Rotate) {
            mStartDirection = mScreenFallback ? Ray.mOrigin + Ray.mDirection * (mStartPivot - Ray.mOrigin).Dot(Ray.mDirection) - mStartPivot : mStartIntersection - mStartPivot;
            mStartDirection = mStartDirection.ProjectedOnPlane(mPlaneNormal).Normalized();

            if (mStartDirection.IsNearlyZero()) {
                return false;
            }

            mPreviousDirection = mStartDirection;

            const Core::FVector Tangent{mPlaneNormal.Cross(mStartDirection)};

            if (Core::Math::Project(mStartPivot + mStartDirection * mDragSize * 74.0f, View.mViewProjection, View.mSize, Start) && Core::Math::Project(mStartPivot + (mStartDirection * 74.0f + Tangent * 10.0f) * mDragSize, View.mViewProjection, View.mSize, End)) {
                mScreenAxis = End - Start;

                const float TangentLength{std::sqrt(mScreenAxis.x * mScreenAxis.x + mScreenAxis.y * mScreenAxis.y)};

                if (TangentLength > 0.00001f) {
                    mScreenAxis /= TangentLength;
                }
            }
        }

        mActiveAxis = Axis;

        return true;
    }

    bool Editor::FGizmoCore::UpdateDrag(const Core::FVector2& Mouse) {
        Core::FMatrix CameraWorld{};

        if (!IsDragging() || !mDragView.mView.TryInverse(CameraWorld)) {
            return false;
        }

        const Core::FVector Right{CameraWorld.GetAxis(0)};
        const Core::FVector Up{CameraWorld.GetAxis(1)};
        const Core::FVector2 MouseDelta{Mouse - mStartMouse};

        if (!std::isfinite(MouseDelta.x) || !std::isfinite(MouseDelta.y)) {
            return false;
        }

        Core::FRay Ray{};

        if (!mDragView.Deproject(Mouse, Ray)) {
            return false;
        }

        const float ScreenMovement{MouseDelta.x * mScreenAxis.x + MouseDelta.y * mScreenAxis.y};

        if (mDragMode == Editor::EGizmoMode::Rotate) {
            if (mScreenFallback) {
                mRawAngle = ScreenMovement / (74.0f * std::clamp(mSettings.mSize, 0.5f, 2.0f));
            } else {
                Core::FVector Intersection{};

                if (!Core::Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, Intersection)) {
                    return false;
                }

                const Core::FVector Direction{(Intersection - mStartPivot).ProjectedOnPlane(mPlaneNormal).Normalized()};

                if (Direction.IsNearlyZero()) {
                    return false;
                }

                mRawAngle += std::atan2(mPlaneNormal.Dot(mPreviousDirection.Cross(Direction)), std::clamp(mPreviousDirection.Dot(Direction), -1.0f, 1.0f));
                mPreviousDirection = Direction;
            }

            mAppliedAngle = Snap(mRawAngle, mSettings.mRotationStep * 1_deg / 1_rad, mSettings.mRotationSnap);

            return true;
        }

        if (mDragMode == Editor::EGizmoMode::Scale) {
            mScaleDelta = Snap(ScreenMovement / 100.0f, mSettings.mScaleStep, mSettings.mScaleSnap);

            return true;
        }

        Core::FVector Delta{};
        Core::FVector Movement{};

        if (mScreenFallback) {
            Movement = mActiveAxis < 3 ? mDragAxes[mActiveAxis] * ScreenMovement * mDragSize : (Right * MouseDelta.x - Up * MouseDelta.y) * mDragSize;
        } else {
            Core::FVector Intersection{};

            if (!Core::Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, Intersection)) {
                return false;
            }

            Movement = Intersection - mStartIntersection;
        }

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            const Core::FVector Axis{mActiveAxis == 6 ? GetUnitAxes()[Index] : mDragAxes[Index]};

            if (mActiveAxis == 6 || Affects(mActiveAxis, Index)) {
                Delta += Axis * Snap(Movement.Dot(Axis), mSettings.mTranslationStep, mSettings.mTranslationSnap);
            }
        }

        if (!Delta.IsFinite()) {
            return false;
        }

        mTranslation = Delta;

        return true;
    }

    void Editor::FGizmoCore::EndDrag() {
        mActiveAxis = -1;
        mHoveredAxis = -1;
    }

    void Editor::FGizmoCore::BuildHandles(const Engine::FSceneView& View, const Editor::FGizmoPlacement& Placement, Engine::FPrimitiveDrawInterface& DrawInterface) const {
        Core::FMatrix CameraWorld{};

        if (Placement.mMode == Editor::EGizmoMode::None || !View.mView.TryInverse(CameraWorld)) {
            return;
        }

        const Core::FVector Right{CameraWorld.GetAxis(0)};
        const Core::FVector Up{CameraWorld.GetAxis(1)};
        const Editor::EGizmoMode Mode{Placement.mMode};
        const Core::FVector Pivot{Placement.mPivot};
        const float Unit{GetSize(View, Pivot)};

        if (Unit <= 0.0f) {
            return;
        }

        const std::array<Core::FVector, 3> Axes{IsDragging() ? mDragAxes : Placement.mAxes};
        const Core::Int32 Highlight{IsDragging() ? mActiveAxis : mHoveredAxis};
        const CoreUObject::FObjectHandle Owner{Placement.mOwner};

        const auto Add{[&](Engine::EEditorMesh Mesh, const Core::FMatrix& World, Core::Int32 Handle, Core::Uint32 Priority, const Core::FVector4& Color, bool Pickable = true) {
            DrawInterface.DrawMesh({Mesh, World, Color, Pickable ? Engine::FHitProxy{Engine::EHitProxyType::GizmoAxis, Owner, Handle} : Engine::FHitProxy{}, Priority});
        }};
        const auto Color{[&](Core::Int32 Handle) {
            if (Handle == Highlight) {
                return Core::FVector4{1.0f, 0.85f, 0.05f, 1.0f};
            }

            return Handle < 3 ? Core::FVector4{AxisColors[Handle], 1.0f} : Handle == 3 ? Core::FVector4{0.8f, 0.8f, 0.2f, 0.35f} : Handle == 4 ? Core::FVector4{0.8f, 0.2f, 0.8f, 0.35f} : Handle == 5 ? Core::FVector4{0.2f, 0.8f, 0.8f, 0.35f} : Core::FVector4{0.8f, 0.8f, 0.8f, 1.0f};
        }};

        if (Mode == Editor::EGizmoMode::Rotate) {
            for (Core::Int32 Index{}; Index < 4; ++Index) {
                const Core::Int32 Handle{Index < 3 ? Index : 6};

                if (IsDragging() && Handle != mActiveAxis) {
                    continue;
                }

                const Core::FVector Normal{Index < 3 ? Axes[Index] : View.mForward};
                const float Radius{Index < 3 ? Unit : Unit * 1.2f};
                Core::FVector First{Index < 3 ? (-View.mForward).ProjectedOnPlane(Normal).Normalized() : Right};
                const bool FaceOn{First.IsNearlyZero()};

                if (First.IsNearlyZero()) {
                    First = Perpendicular(Normal);
                }

                const Core::FVector Second{Normal.Cross(First).Normalized()};
                const bool Full{Index == 3 || IsDragging() || FaceOn};
                Add(Full ? Engine::EEditorMesh::Ring : Engine::EEditorMesh::HalfRing, Basis(First * Radius, Second * Radius, Normal * Radius, Pivot), Handle, 2, Color(Handle));

                if (!Full) {
                    Core::FVector4 Back{Color(Handle) * 0.25f};
                    Back.w = 0.45f;
                    Add(Engine::EEditorMesh::HalfRing, Basis(-First * Radius, -Second * Radius, Normal * Radius, Pivot), Handle, 2, Back, false);
                }
            }

            return;
        }

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            if (std::abs(Axes[Index].Dot(View.mForward)) < 0.995f && (!IsDragging() || mActiveAxis == Index)) {
                Add(Mode == Editor::EGizmoMode::Scale ? Engine::EEditorMesh::Hammer : Engine::EEditorMesh::Arrow, Basis(Axes[(Index + 1) % 3] * Unit, Axes[(Index + 2) % 3] * Unit, Axes[Index] * Unit, Pivot), Index, 2, Color(Index));
            }
        }

        const std::array<std::array<Core::Int32, 2>, 3> Pairs{{{0, 1}, {0, 2}, {1, 2}}};

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            const Core::FVector First{Axes[Pairs[Index][0]]};
            const Core::FVector Second{Axes[Pairs[Index][1]]};

            if (std::abs(First.Cross(Second).Dot(View.mForward)) < 0.08f || (IsDragging() && mActiveAxis != Index + 3)) {
                continue;
            }

            const float Near{Mode == Editor::EGizmoMode::Scale ? 18.0f : 20.0f};
            const float Width{Mode == Editor::EGizmoMode::Scale ? 30.0f : 16.0f};
            Add(Mode == Editor::EGizmoMode::Scale ? Engine::EEditorMesh::Triangle : Engine::EEditorMesh::Quad, Basis(First * Width * Unit, Second * Width * Unit, First.Cross(Second) * Unit, Pivot + (First + Second) * Near * Unit), Index + 3, 1, Color(Index + 3));
        }

        const Core::Int32 Center{Mode == Editor::EGizmoMode::Scale ? 7 : 6};

        if (!IsDragging() || mActiveAxis == Center) {
            const Core::FMatrix World{Mode == Editor::EGizmoMode::Scale ? Basis(Axes[0] * 8.0f * Unit, Axes[1] * 8.0f * Unit, Axes[2] * 8.0f * Unit, Pivot) : Basis(Right * 10.0f * Unit, Up * 10.0f * Unit, View.mForward * Unit, Pivot - (Right + Up) * 5.0f * Unit)};
            Add(Mode == Editor::EGizmoMode::Scale ? Engine::EEditorMesh::Cube : Engine::EEditorMesh::Quad, World, Center, 0, Color(Center));
        }
    }

    void Editor::FGizmoCore::DrawFeedback(Engine::FPrimitiveDrawInterface& DrawInterface) const {
        const Core::FVector Color{1.0f, 0.85f, 0.05f};

        if (!IsDragging()) {
            return;
        }

        if (mDragMode == Editor::EGizmoMode::Rotate) {
            const float Angle{Core::Math::Clamp(mAppliedAngle * 1_rad, -1_turn, 1_turn) / 1_rad};
            const float Radius{mDragSize * 74.0f};
            DrawInterface.DrawLine(mStartPivot, mStartPivot + mStartDirection * Radius, {0.8f, 0.8f, 0.8f});
            DrawInterface.DrawLine(mStartPivot, mStartPivot + Core::FQuat{mPlaneNormal, Angle} * mStartDirection * Radius, Color);

            for (Core::Int32 Index{}; Index < 64; ++Index) {
                DrawInterface.DrawLine(mStartPivot + Core::FQuat{mPlaneNormal, Angle * Index / 64.0f} * mStartDirection * Radius, mStartPivot + Core::FQuat{mPlaneNormal, Angle * (Index + 1) / 64.0f} * mStartDirection * Radius, Color);
            }
        } else if (mDragMode == Editor::EGizmoMode::Translate) {
            DrawInterface.DrawLine(mStartPivot, mStartPivot + mTranslation, Color);
        }
    }

    bool Editor::FGizmoCore::IsDragging() const {
        return mActiveAxis >= 0;
    }

    Core::Int32 Editor::FGizmoCore::GetActiveAxis() const {
        return mActiveAxis;
    }

    Core::Int32 Editor::FGizmoCore::GetHoveredAxis() const {
        return mHoveredAxis;
    }

    void Editor::FGizmoCore::SetHoveredAxis(Core::Int32 Axis) {
        mHoveredAxis = Axis;
    }

    Editor::EGizmoMode Editor::FGizmoCore::GetDragMode() const {
        return mDragMode;
    }

    const Core::FVector2& Editor::FGizmoCore::GetStartMouse() const {
        return mStartMouse;
    }

    const Core::FVector& Editor::FGizmoCore::GetStartPivot() const {
        return mStartPivot;
    }

    const std::array<Core::FVector, 3>& Editor::FGizmoCore::GetDragAxes() const {
        return mDragAxes;
    }

    const Core::FVector& Editor::FGizmoCore::GetTranslation() const {
        return mTranslation;
    }

    Core::FQuat Editor::FGizmoCore::GetRotation() const {
        return Core::FQuat{mPlaneNormal, mAppliedAngle};
    }

    float Editor::FGizmoCore::GetScaleDelta() const {
        return mScaleDelta;
    }

    const Editor::FGizmoSettings* Editor::FGizmoCore::GetSettings() const {
        return &mSettings;
    }

    void Editor::FGizmoCore::SetSettings(const Editor::FGizmoSettings& Settings) {
        mSettings = Settings;
    }

}
