#include "pch.h"
#include "Editor/View/FTransformGizmo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numbers>
#include "Engine/Serialization/FWorldSerializer.h"
#include "ImGui/imgui.h"
#include "World/Component/UMeshComponent.h"
#include "World/UWorld.h"

namespace {
    constexpr float Pi{std::numbers::pi_v<float>};
    constexpr float AxisStart{11.0f};
    constexpr float TranslateAxisLength{87.5f};
    constexpr float ScaleAxisLength{84.5f};
    constexpr float RingRadius{74.0f};
    constexpr Uint32 RingSegments{96};

    bool ProjectPoint(const FVector3& Point, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, FVector2& Screen) {
        FVector3 Clip{};

        if (Viewport.Width <= 0.0f || Viewport.Height <= 0.0f || !Camera.mViewProjection.TransformCoord(Point, Clip) || !std::isfinite(Clip.mX) || !std::isfinite(Clip.mY)) {
            return false;
        }

        if (std::abs(Camera.mProjection.m_[2][3]) > 0.000001f && FVector3::Transform(Point, Camera.mView).mZ <= 0.0f) {
            return false;
        }

        Screen = {Viewport.TopLeftX + (Clip.mX + 1.0f) * Viewport.Width * 0.5f, Viewport.TopLeftY + (1.0f - Clip.mY) * Viewport.Height * 0.5f};
        return true;
    }

    std::optional<FRay> MakeRay(const POINT& Position, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport) {
        FMatrix Inverse{};

        if (Viewport.Width <= 0.0f || Viewport.Height <= 0.0f || !Camera.mViewProjection.TryInverse(Inverse)) {
            return std::nullopt;
        }

        const float X{2.0f * (static_cast<float>(Position.x) - Viewport.TopLeftX) / Viewport.Width - 1.0f};
        const float Y{1.0f - 2.0f * (static_cast<float>(Position.y) - Viewport.TopLeftY) / Viewport.Height};
        FVector3 Start{};
        FVector3 End{};

        if (!Inverse.TransformCoord({X, Y, 0.0f}, Start) || !Inverse.TransformCoord({X, Y, 1.0f}, End)) {
            return std::nullopt;
        }

        FVector3 Direction{End - Start};

        if (Direction.LengthSquared() < 0.000001f) {
            return std::nullopt;
        }

        Direction.Normalize();
        return FRay{Start.ToSimpleMath(), Direction.ToSimpleMath()};
    }

    bool IntersectPlane(const FRay& Ray, const FVector3& Pivot, const FVector3& Normal, FVector3& Point) {
        const FVector3 Direction{Ray.direction};
        const float Denominator{Direction.Dot(Normal)};

        if (!std::isfinite(Denominator) || std::abs(Denominator) < 0.03f) {
            return false;
        }

        const float Distance{(Pivot - FVector3{Ray.position}).Dot(Normal) / Denominator};

        if (!std::isfinite(Distance) || Distance < 0.0f) {
            return false;
        }

        Point = FVector3{Ray.position} + Direction * Distance;
        return std::isfinite(Point.mX) && std::isfinite(Point.mY) && std::isfinite(Point.mZ);
    }

    float ScreenDot(const FVector2& First, const FVector2& Second) {
        return First.mX * Second.mX + First.mY * Second.mY;
    }

    float SegmentDistance(const FVector2& Point, const FVector2& Start, const FVector2& End, float& Parameter) {
        const FVector2 Segment{End - Start};
        const float LengthSquared{Segment.LengthSquared()};

        Parameter = LengthSquared > 0.000001f ? std::clamp(ScreenDot(Point - Start, Segment) / LengthSquared, 0.0f, 1.0f) : 0.0f;
        return (Point - (Start + Segment * Parameter)).Length();
    }

    bool InsidePolygon(const FVector2& Point, const std::array<FVector2, 4>& Polygon, Uint32 Count) {
        bool Positive{};
        bool Negative{};
        float Area{};

        for (Uint32 Index{}; Index < Count; ++Index) {
            const FVector2& Start{Polygon[Index]};
            const FVector2& End{Polygon[(Index + 1) % Count]};
            const float Cross{(End.mX - Start.mX) * (Point.mY - Start.mY) - (End.mY - Start.mY) * (Point.mX - Start.mX)};

            Positive = Positive || Cross > 0.001f;
            Negative = Negative || Cross < -0.001f;
            Area += Start.mX * End.mY - Start.mY * End.mX;
        }

        return std::abs(Area) > 8.0f && !(Positive && Negative);
    }

    float SnapValue(float Value, float Step, bool Enabled) {
        return Enabled && std::isfinite(Step) && Step > 0.0f ? std::round(Value / Step) * Step : Value;
    }

    float LimitScale(float Initial, float Desired) {
        return Initial < 0.0f ? std::min(-0.001f, Desired) : std::max(0.001f, Desired);
    }

    FQuat AlignDirection(const FVector3& From, const FVector3& To) {
        const float Dot{std::clamp(From.Dot(To), -1.0f, 1.0f)};
        FVector3 Axis{From.Cross(To)};

        if (Axis.LengthSquared() < 0.000001f) {
            if (Dot > 0.0f) {
                return {};
            }

            Axis = From.Cross(std::abs(From.mZ) < 0.9f ? FVector3::UnitZ : FVector3::UnitX);
        }

        Axis.Normalize();
        return FQuat::CreateFromAxisAngle(Axis, std::acos(Dot));
    }
}

void FTransformGizmo::Initialize(ID3D11Device* Device, FAssetRegistry& AssetRegistry, FWorldEditorContext& InEditorContext) {
    UNREFERENCED_PARAMETER(Device);
    mAssetRegistry = &AssetRegistry;
    mEditorContext = &InEditorContext;
    RefreshAssetHandles();
}

void FTransformGizmo::ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool BMouseCapturedByUi, bool BKeyboardCapturedByUi) {
    if (mEditorContext == nullptr || !mEditorContext->IsWorldEditingAllowed()) {
        EndDrag(true);
        mHoveredHandle = EHandle::None;
        return;
    }

    mVertexSnap = KeyboardInput.IsHeld('V');

    if (mDragSession.has_value()) {
        const EKeyState State{MouseInput.GetKeyState(mDragSession->mButton)};

        if (MouseInput.WasCaptureLost() || KeyboardInput.GetKeyState(VK_ESCAPE) == EKeyState::Pressed || mEditorContext->GetWorld() != mDragSession->mWorld || mEditorContext->GetSelectionRevision() != mDragSession->mSelectionRevision) {
            EndDrag(true);
            return;
        }

        if (State == EKeyState::Down || State == EKeyState::Pressed || State == EKeyState::Released) {
            UpdateDrag(MouseInput.GetDragCapture(mDragSession->mButton).mCurrent, mVertexSnap, mEditorContext->GetEditorSettings().mSurfaceSnapEnabled);
        }

        if (State == EKeyState::Released || State == EKeyState::None) {
            EndDrag(State == EKeyState::None);
        }

        return;
    }

    if (!BKeyboardCapturedByUi && MouseInput.GetKeyState(Right) != EKeyState::Pressed && MouseInput.GetKeyState(Right) != EKeyState::Down && !KeyboardInput.IsHeld(VK_CONTROL) && !KeyboardInput.IsHeld(VK_MENU)) {
        if (KeyboardInput.GetKeyState('Q') == EKeyState::Pressed) {
            mEditorContext->SetGizmoMode(EGizmoMode::None);
        } else if (KeyboardInput.GetKeyState('W') == EKeyState::Pressed) {
            mEditorContext->SetGizmoMode(EGizmoMode::Translate);
        } else if (KeyboardInput.GetKeyState('E') == EKeyState::Pressed) {
            mEditorContext->SetGizmoMode(EGizmoMode::Rotate);
        } else if (KeyboardInput.GetKeyState('R') == EKeyState::Pressed) {
            mEditorContext->SetGizmoMode(EGizmoMode::Scale);
        } else if (KeyboardInput.GetKeyState(VK_SPACE) == EKeyState::Pressed) {
            mEditorContext->SetGizmoMode(mEditorContext->GetGizmoMode() == EGizmoMode::None ? EGizmoMode::Translate : static_cast<EGizmoMode>((static_cast<Uint8>(mEditorContext->GetGizmoMode()) + 1) % 3));
        }

        if (KeyboardInput.GetKeyState(VK_TAB) == EKeyState::Pressed && mEditorContext->GetGizmoMode() != EGizmoMode::Scale) {
            mEditorContext->SetGizmoCoordinateSpace(mEditorContext->GetGizmoCoordinateSpace() == EGizmoCoordinateSpace::World ? EGizmoCoordinateSpace::Local : EGizmoCoordinateSpace::World);
        }

        FEditorSettings Settings{mEditorContext->GetEditorSettings()};

        if (KeyboardInput.GetKeyState('X') == EKeyState::Pressed) {
            switch (mEditorContext->GetGizmoMode()) {
                case EGizmoMode::Translate: Settings.mGridSnapEnabled = !Settings.mGridSnapEnabled; break;
                case EGizmoMode::Rotate: Settings.mRotationSnapEnabled = !Settings.mRotationSnapEnabled; break;
                case EGizmoMode::Scale: Settings.mScaleSnapEnabled = !Settings.mScaleSnapEnabled; break;
                default: break;
            }
        }

        if (KeyboardInput.GetKeyState('Z') == EKeyState::Pressed) {
            Settings.mSurfaceSnapEnabled = !Settings.mSurfaceSnapEnabled;
        }

        mEditorContext->SetEditorSettings(Settings);
        Update(mLastCamera, mLastViewport);
    }

    mHoveredHandle = !BMouseCapturedByUi && mVisible ? HitTest(MouseInput.GetPosition()) : EHandle::None;
    mSnapPosition.reset();

    if (mVertexSnap && !BMouseCapturedByUi) {
        FVector3 Vertex{};

        if (FindVertex(MouseInput.GetPosition(), Vertex)) {
            mSnapPosition = Vertex;
        }
    }

    if (BMouseCapturedByUi || !mVisible) {
        return;
    }

    const bool PivotOnly{MouseInput.GetKeyState(Middle) == EKeyState::Pressed && mEditorContext->GetGizmoMode() == EGizmoMode::Translate};
    const EMouseSide Button{PivotOnly ? Middle : Left};

    if (MouseInput.GetKeyState(Button) != EKeyState::Pressed) {
        return;
    }

    const FMouseInput::DragCapture& Capture{MouseInput.GetDragCapture(Button)};
    const EHandle PressedHandle{HitTest(Capture.mStart)};
    const bool Duplicate{!PivotOnly && KeyboardInput.IsHeld(VK_MENU) && mEditorContext->GetGizmoMode() == EGizmoMode::Translate};

    if (PressedHandle != EHandle::None && BeginDrag(PressedHandle, Capture.mStart, Button, PivotOnly, Duplicate)) {
        MouseInput.Consume(Button);
        UpdateDrag(Capture.mCurrent, mVertexSnap, mEditorContext->GetEditorSettings().mSurfaceSnapEnabled);
    }
}

void FTransformGizmo::Update(const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport) {
    if (mDragSession.has_value() && (mEditorContext->GetWorld() != mDragSession->mWorld || !mEditorContext->IsWorldEditingAllowed() || mEditorContext->GetSelectionRevision() != mDragSession->mSelectionRevision)) {
        EndDrag(true);
    }
    mLastCamera = Camera;
    mLastViewport = Viewport;
    mHasCamera = Viewport.Width > 0.0f && Viewport.Height > 0.0f;
    mVisible = false;
    mPickShapes.clear();
    mMeshes.clear();
    mLines.Clear();

    if (!mHasCamera || mEditorContext == nullptr || !mEditorContext->IsWorldEditingAllowed() || mEditorContext->GetGizmoMode() == EGizmoMode::None) {
        return;
    }

    const TArray<USceneComponent*> Targets{mEditorContext->GetSelectedTransformTargets()};
    USceneComponent* Target{Targets.empty() ? nullptr : Targets.back()};

    if (Target == nullptr) {
        return;
    }

    FMatrix CameraWorld{};

    if (!Camera.mView.TryInverse(CameraWorld)) {
        return;
    }

    const bool Perspective{std::abs(Camera.mProjection.m_[2][3]) > 0.000001f};
    mPivot = mDragSession.has_value() ? mDragSession->mPivot + mDragSession->mTranslation : mEditorContext->GetGizmoPivot();
    const float Depth{FVector3::Transform(mPivot, Camera.mView).mZ};
    const float ProjectionScale{std::abs(Camera.mProjection.m_[1][1])};

    if (ProjectionScale < 0.000001f || (Perspective && Depth <= 0.0f)) {
        return;
    }

    mUnitsPerPixel = 2.0f * (Perspective ? Depth : 1.0f) / (Viewport.Height * ProjectionScale);
    mSize = std::clamp(mEditorContext->GetEditorSettings().mGizmoSize, 0.5f, 2.0f);

    if (!std::isfinite(mUnitsPerPixel) || mUnitsPerPixel <= 0.0f) {
        return;
    }

    mViewRight = FVector3::TransformNormal(FVector3::UnitX, CameraWorld);
    mViewUp = FVector3::TransformNormal(FVector3::UnitY, CameraWorld);
    mViewDirection = FVector3::TransformNormal(FVector3::UnitZ, CameraWorld);
    mViewRight.Normalize();
    mViewUp.Normalize();
    mViewDirection.Normalize();
    mBasis = FMatrix::Identity;

    if (mDragSession.has_value()) {
        mBasis = mDragSession->mBasis;
    } else if (mEditorContext->GetGizmoCoordinateSpace() == EGizmoCoordinateSpace::Local || mEditorContext->GetGizmoMode() == EGizmoMode::Scale) {
        mBasis = FMatrix::CreateFromQuaternion(Target->GetComponentTransform().GetRotationQuaternion());
    }

    const std::array<FVector3, 3> UnitAxes{FVector3::UnitX, FVector3::UnitY, FVector3::UnitZ};

    for (Uint32 Index{}; Index < 3; ++Index) {
        mAxes[Index] = FVector3::TransformNormal(UnitAxes[Index], mBasis);
        mAxes[Index].Normalize();
    }

    mVisible = true;
    RefreshAssetHandles();
    BuildHandles();
}

void FTransformGizmo::BuildHandles() {
    if (mEditorContext->GetGizmoMode() == EGizmoMode::Rotate) {
        BuildRotation();
        return;
    }

    BuildAxes();
    BuildPlanes();
    const float Radius{5.0f * mUnitsPerPixel * mSize};
    const EHandle Handle{mEditorContext->GetGizmoMode() == EGizmoMode::Scale ? EHandle::Uniform : EHandle::Screen};
    const std::array<FVector3, 4> Points{mPivot - mViewRight * Radius - mViewUp * Radius, mPivot + mViewRight * Radius - mViewUp * Radius, mPivot + mViewRight * Radius + mViewUp * Radius, mPivot - mViewRight * Radius + mViewUp * Radius};

    AddPolygon(Handle, Points, 4, GetHandleColor(Handle), 0);

    if (Handle == EHandle::Uniform && mCubeMesh) {
        const FMatrix World{FMatrix::CreateScale(Radius * 1.6f, Radius * 1.6f, Radius * 1.6f) * mBasis * FMatrix::CreateTranslation(mPivot)};
        mMeshes.push_back({World, FMeshSceneData{mCubeMesh, GetHandleMaterial(Handle), mGizmoPipeline}});
    }
}

void FTransformGizmo::BuildAxes() {
    const float Unit{mUnitsPerPixel * mSize};
    const bool Scale{mEditorContext->GetGizmoMode() == EGizmoMode::Scale};
    const float Length{Scale ? ScaleAxisLength : TranslateAxisLength};
    const FAssetHandle Mesh{Scale ? mScaleArrowMesh : mArrowMesh};
    const std::array<FMatrix, 3> Rotations{FMatrix::CreateRotationY(Pi * 0.5f), FMatrix::CreateRotationX(-Pi * 0.5f), FMatrix::Identity};

    for (Uint32 Index{}; Index < 3; ++Index) {
        const EHandle Handle{static_cast<EHandle>(Index + 1)};

        if (std::abs(mAxes[Index].Dot(mViewDirection)) > 0.995f) {
            continue;
        }

        const FVector3 Start{mPivot + mAxes[Index] * AxisStart * Unit};
        const FVector3 End{mPivot + mAxes[Index] * Length * Unit};
        mPickShapes.push_back({Handle, {Start, End, FVector3{}, FVector3{}}, 2, 8.0f, 2});
        const FMatrix World{FMatrix::CreateScale(Unit, Unit, Unit) * Rotations[Index] * mBasis * FMatrix::CreateTranslation(mPivot)};

        if (Mesh && mGizmoPipeline && (!mDragSession.has_value() || mDragSession->mHandle == Handle)) {
            mMeshes.push_back({World, FMeshSceneData{Mesh, GetHandleMaterial(Handle), mGizmoPipeline}});
        }
    }
}

void FTransformGizmo::BuildPlanes() {
    const float Unit{mUnitsPerPixel * mSize};
    const bool Scale{mEditorContext->GetGizmoMode() == EGizmoMode::Scale};
    constexpr std::array<std::array<Uint32, 2>, 3> AxisPairs{std::array<Uint32, 2>{0, 1}, std::array<Uint32, 2>{0, 2}, std::array<Uint32, 2>{1, 2}};

    for (Uint32 Index{}; Index < 3; ++Index) {
        const EHandle Handle{static_cast<EHandle>(static_cast<Uint8>(EHandle::XY) + Index)};
        const FVector3 First{mAxes[AxisPairs[Index][0]]};
        const FVector3 Second{mAxes[AxisPairs[Index][1]]};

        if (std::abs(First.Cross(Second).Dot(mViewDirection)) < 0.08f) {
            continue;
        }

        const float Near{Scale ? 18.0f : 20.0f};
        const float Far{Scale ? 48.0f : 36.0f};
        const std::array<FVector3, 4> Points{mPivot + (First + Second) * Near * Unit, mPivot + (First * Far + Second * Near) * Unit, mPivot + (First + Second) * Far * Unit, mPivot + (First * Near + Second * Far) * Unit};
        const std::array<FVector3, 4> Triangle{Points[0], Points[1], Points[3], FVector3{}};
        AddPolygon(Handle, Scale ? Triangle : Points, Scale ? 3 : 4, GetHandleColor(Handle), 1);
    }
}

void FTransformGizmo::BuildRotation() {
    const float Radius{RingRadius * mUnitsPerPixel * mSize};
    const EHandle Active{mDragSession.has_value() ? mDragSession->mHandle : EHandle::None};

    for (Uint32 Index{}; Index < 4; ++Index) {
        const EHandle Handle{Index < 3 ? static_cast<EHandle>(Index + 1) : EHandle::Screen};

        if (Active != EHandle::None && Active != Handle) {
            continue;
        }

        const FVector3 First{Index < 3 ? mAxes[(Index + 1) % 3] : mViewRight};
        const FVector3 Second{Index < 3 ? mAxes[(Index + 2) % 3] : mViewUp};
        const float CurrentRadius{Radius * (Index < 3 ? 1.0f : 1.2f)};

        for (Uint32 Segment{}; Segment < RingSegments; ++Segment) {
            const float StartAngle{2.0f * Pi * static_cast<float>(Segment) / RingSegments};
            const float EndAngle{2.0f * Pi * static_cast<float>(Segment + 1) / RingSegments};
            const FVector3 Start{First * std::cos(StartAngle) + Second * std::sin(StartAngle)};
            const FVector3 End{First * std::cos(EndAngle) + Second * std::sin(EndAngle)};
            const bool Front{Index == 3 || (Start + End).Dot(mViewDirection) <= 0.001f || Active == Handle};
            FVector4 Color{GetHandleColor(Handle)};

            if (!Front) {
                Color = {Color.mX * 0.25f, Color.mY * 0.25f, Color.mZ * 0.25f, 0.45f};
            }

            AddSegment(Handle, mPivot + Start * CurrentRadius, mPivot + End * CurrentRadius, Color, Active == Handle ? 3.5f : 2.5f, Front);
        }
    }
}

void FTransformGizmo::AddSegment(EHandle Handle, const FVector3& Start, const FVector3& End, const FVector4& Color, float Width, bool Pickable) {
    mLines.AddLine(Start, End, Color, Width, ELineDepthMode::Overlay);

    if (Pickable) {
        mPickShapes.push_back({Handle, {Start, End, FVector3{}, FVector3{}}, 2, 8.0f, 2});
    }
}

void FTransformGizmo::AddPolygon(EHandle Handle, const std::array<FVector3, 4>& Points, Uint32 Count, const FVector4& Color, Uint32 Priority) {
    mPickShapes.push_back({Handle, Points, Count, 3.0f, Priority});

    if (Priority == 1) {
        FVector4 Fill{Color};
        Fill.mW = Handle == (mDragSession.has_value() ? mDragSession->mHandle : mHoveredHandle) ? 0.3f : 0.12f;

        for (Uint32 Index{1}; Index < 20; ++Index) {
            const float Fraction{static_cast<float>(Index) / 20.0f};
            const FVector3 Start{Points[0] + (Points[1] - Points[0]) * Fraction};
            const FVector3 End{Count == 3 ? Points[0] + (Points[2] - Points[0]) * Fraction : Points[3] + (Points[2] - Points[3]) * Fraction};
            mLines.AddLine(Start, End, Fill, 1.5f, ELineDepthMode::Overlay);
        }
    }

    for (Uint32 Index{}; Index < Count; ++Index) {
        mLines.AddLine(Points[Index], Points[(Index + 1) % Count], Color, 2.0f, ELineDepthMode::Overlay);
    }
}

FTransformGizmo::EHandle FTransformGizmo::HitTest(const POINT& Position) const {
    const FVector2 Mouse{static_cast<float>(Position.x), static_cast<float>(Position.y)};
    EHandle Best{EHandle::None};
    Uint32 BestPriority{UINT32_MAX};
    float BestDistance{std::numeric_limits<float>::max()};

    for (const FPickShape& Shape : mPickShapes) {
        std::array<FVector2, 4> Points{};
        bool Valid{true};

        for (Uint32 Index{}; Index < Shape.mCount; ++Index) {
            Valid = ProjectPoint(Shape.mPoints[Index], mLastCamera, mLastViewport, Points[Index]) && Valid;
        }

        if (!Valid) {
            continue;
        }

        float Distance{std::numeric_limits<float>::max()};

        if (Shape.mCount > 2 && InsidePolygon(Mouse, Points, Shape.mCount)) {
            Distance = 0.0f;
        } else {
            const Uint32 EdgeCount{Shape.mCount == 2 ? 1 : Shape.mCount};

            for (Uint32 Index{}; Index < EdgeCount; ++Index) {
                float Parameter{};
                Distance = std::min(Distance, SegmentDistance(Mouse, Points[Index], Points[(Index + 1) % Shape.mCount], Parameter));
            }
        }

        if (Distance <= Shape.mTolerance && (Shape.mPriority < BestPriority || (Shape.mPriority == BestPriority && Distance < BestDistance))) {
            Best = Shape.mHandle;
            BestPriority = Shape.mPriority;
            BestDistance = Distance;
        }
    }

    return Best;
}

bool FTransformGizmo::BeginDrag(EHandle Handle, const POINT& Position, EMouseSide Button, bool PivotOnly, bool Duplicate) {
    const std::optional<FRay> Ray{MakeRay(Position, mLastCamera, mLastViewport)};
    const TArray<USceneComponent*> Targets{mEditorContext->GetSelectedTransformTargets()};

    if (!Ray.has_value() || Targets.empty()) {
        return false;
    }

    FDragSession Session{};
    Session.mWorld = mEditorContext->GetWorld();
    Session.mCamera = mLastCamera;
    Session.mViewport = mLastViewport;
    Session.mBasis = mBasis;
    Session.mAxes = mAxes;
    Session.mPivot = mPivot;
    Session.mOriginalPivot = mEditorContext->GetGizmoPivot();
    Session.mHadPivot = mEditorContext->HasGizmoPivot();
    Session.mStartMouse = Position;
    Session.mUnitsPerPixel = mUnitsPerPixel;
    Session.mSelectionRevision = mEditorContext->GetSelectionRevision();
    Session.mHandle = Handle;
    Session.mMode = mEditorContext->GetGizmoMode();
    Session.mButton = Button;
    Session.mPivotOnly = PivotOnly;
    Session.mDuplicatePending = Duplicate;
    Session.mOriginalComponent.Set(mEditorContext->GetSelectedComponent());

    for (USceneComponent* Target : Targets) {
        Session.mTargets.push_back({TObjectRef<USceneComponent>{Target}, Target->GetRelativeTransform(), Target->GetComponentTransform()});
    }

    for (AActor* Actor : mEditorContext->GetSelectedActors()) {
        Session.mOriginalSelection.push_back(TObjectRef<AActor>{Actor});
    }

    FVector3 Axis{GetHandleAxis(Handle)};
    Session.mPlaneNormal = mViewDirection;

    if (Handle == EHandle::XY) {
        Session.mPlaneNormal = mAxes[0].Cross(mAxes[1]);
        Axis = mAxes[0] + mAxes[1];
    } else if (Handle == EHandle::XZ) {
        Session.mPlaneNormal = mAxes[0].Cross(mAxes[2]);
        Axis = mAxes[0] + mAxes[2];
    } else if (Handle == EHandle::YZ) {
        Session.mPlaneNormal = mAxes[1].Cross(mAxes[2]);
        Axis = mAxes[1] + mAxes[2];
    } else if (Session.mMode == EGizmoMode::Rotate) {
        Session.mPlaneNormal = Handle == EHandle::Screen ? mViewDirection : Axis;
    } else if (Handle >= EHandle::X && Handle <= EHandle::Z) {
        Session.mPlaneNormal = mViewDirection - Axis * mViewDirection.Dot(Axis);
    }

    if (Session.mPlaneNormal.LengthSquared() < 0.000001f) {
        return false;
    }

    Session.mPlaneNormal.Normalize();
    FVector2 ScreenPivot{};
    FVector2 ScreenEnd{};
    ProjectPoint(mPivot, Session.mCamera, Session.mViewport, ScreenPivot);
    ProjectPoint(mPivot + Axis * 100.0f * mUnitsPerPixel, Session.mCamera, Session.mViewport, ScreenEnd);
    Session.mScreenAxis = ScreenEnd - ScreenPivot;

    if (Handle == EHandle::Uniform || Handle == EHandle::Screen || Session.mMode == EGizmoMode::Rotate) {
        Session.mScreenAxis = {0.70710678f, -0.70710678f};
    } else if (Session.mScreenAxis.LengthSquared() > 0.000001f) {
        Session.mScreenAxis.Normalize();
    } else {
        return false;
    }

    Session.mScreenFallback = std::abs(FVector3{Ray->direction}.Dot(Session.mPlaneNormal)) < 0.12f || !IntersectPlane(*Ray, mPivot, Session.mPlaneNormal, Session.mStartIntersection);

    if (Session.mMode == EGizmoMode::Rotate) {
        FVector2 PickTangent{};
        Session.mStartDirection = Session.mStartIntersection - mPivot;

        if (Session.mScreenFallback || Session.mStartDirection.LengthSquared() < 0.000001f) {
            float BestDistance{std::numeric_limits<float>::max()};
            const FVector2 Mouse{static_cast<float>(Position.x), static_cast<float>(Position.y)};

            for (const FPickShape& Shape : mPickShapes) {
                FVector2 Start{};
                FVector2 End{};

                if (Shape.mHandle != Handle || Shape.mCount != 2 || !ProjectPoint(Shape.mPoints[0], Session.mCamera, Session.mViewport, Start) || !ProjectPoint(Shape.mPoints[1], Session.mCamera, Session.mViewport, End)) {
                    continue;
                }

                float Parameter{};
                const float Distance{SegmentDistance(Mouse, Start, End, Parameter)};

                if (Distance < BestDistance) {
                    BestDistance = Distance;
                    Session.mStartDirection = Shape.mPoints[0] + (Shape.mPoints[1] - Shape.mPoints[0]) * Parameter - mPivot;
                    PickTangent = End - Start;
                }
            }
        }

        Session.mStartDirection -= Session.mPlaneNormal * Session.mStartDirection.Dot(Session.mPlaneNormal);

        if (Session.mStartDirection.LengthSquared() < 0.000001f) {
            return false;
        }

        Session.mStartDirection.Normalize();
        Session.mPreviousDirection = Session.mStartDirection;
        const FVector3 Tangent{Session.mPlaneNormal.Cross(Session.mStartDirection)};
        ProjectPoint(mPivot + Session.mStartDirection * RingRadius * mUnitsPerPixel, Session.mCamera, Session.mViewport, ScreenPivot);
        ProjectPoint(mPivot + (Session.mStartDirection * RingRadius + Tangent * 10.0f) * mUnitsPerPixel, Session.mCamera, Session.mViewport, ScreenEnd);
        Session.mScreenAxis = ScreenEnd - ScreenPivot;

        if (Session.mScreenAxis.LengthSquared() < 0.000001f) {
            Session.mScreenAxis = PickTangent;

            if (Session.mScreenAxis.LengthSquared() < 0.000001f) {
                return false;
            }
        }

        Session.mScreenAxis.Normalize();
    }

    mDragSession = std::move(Session);
    mEditorContext->SetGizmoInteracting(true);
    return true;
}

void FTransformGizmo::UpdateDrag(const POINT& Position, bool VertexSnap, bool SurfaceSnap) {
    if (!mDragSession.has_value()) {
        return;
    }

    FDragSession& Session{*mDragSession};
    const FVector2 MouseDelta{static_cast<float>(Position.x - Session.mStartMouse.x), static_cast<float>(Position.y - Session.mStartMouse.y)};

    if (Session.mDuplicatePending && MouseDelta.LengthSquared() > 9.0f) {
        if (!DuplicateSelection()) {
            EndDrag(true);
            return;
        }
    }

    for (const FTargetState& State : Session.mTargets) {
        if (State.mTarget.Get() == nullptr) {
            EndDrag(true);
            return;
        }
    }

    const FEditorSettings Settings{mEditorContext->GetEditorSettings()};
    const std::optional<FRay> Ray{MakeRay(Position, Session.mCamera, Session.mViewport)};

    if (!Ray.has_value() || !std::isfinite(MouseDelta.mX) || !std::isfinite(MouseDelta.mY)) {
        return;
    }

    if (Session.mMode == EGizmoMode::Rotate) {
        if (Session.mScreenFallback) {
            Session.mRawAngle = ScreenDot(MouseDelta, Session.mScreenAxis) / RingRadius;
        } else {
            FVector3 Intersection{};

            if (!IntersectPlane(*Ray, Session.mPivot, Session.mPlaneNormal, Intersection)) {
                return;
            }

            FVector3 Direction{Intersection - Session.mPivot};
            Direction -= Session.mPlaneNormal * Direction.Dot(Session.mPlaneNormal);

            if (Direction.LengthSquared() < 0.000001f) {
                return;
            }

            Direction.Normalize();
            const float SinAngle{Session.mPlaneNormal.Dot(Session.mPreviousDirection.Cross(Direction))};
            const float CosAngle{std::clamp(Session.mPreviousDirection.Dot(Direction), -1.0f, 1.0f)};
            Session.mRawAngle += std::atan2(SinAngle, CosAngle);
            Session.mPreviousDirection = Direction;
        }

        Session.mAppliedAngle = SnapValue(Session.mRawAngle, Settings.mRotationSnapDegrees * Pi / 180.0f, Settings.mRotationSnapEnabled);
        const FQuat Rotation{FQuat::CreateFromAxisAngle(Session.mPlaneNormal, Session.mAppliedAngle)};

        for (const FTargetState& State : Session.mTargets) {
            FTransform Desired{State.mWorld};
            Desired.SetPosition(Session.mPivot + Rotation.RotateVector(State.mWorld.GetPosition() - Session.mPivot));
            Desired.SetRotation(Rotation * State.mWorld.GetRotationQuaternion());
            State.mTarget->SetWorldTransform(Desired);
        }

        return;
    }

    if (Session.mMode == EGizmoMode::Scale) {
        const float RawDelta{ScreenDot(MouseDelta, Session.mScreenAxis) / 100.0f};
        const float Delta{SnapValue(RawDelta, Settings.mScaleSnapStep, Settings.mScaleSnapEnabled)};
        Session.mScaleDelta = {};
        Session.mUniformFactor = 1.0f;

        if (Session.mHandle == EHandle::Uniform) {
            Session.mUniformFactor = std::max(0.001f, 1.0f + Delta);
        } else {
            if (Session.mHandle == EHandle::X || Session.mHandle == EHandle::XY || Session.mHandle == EHandle::XZ) {
                Session.mScaleDelta.mX = Delta;
            }

            if (Session.mHandle == EHandle::Y || Session.mHandle == EHandle::XY || Session.mHandle == EHandle::YZ) {
                Session.mScaleDelta.mY = Delta;
            }

            if (Session.mHandle == EHandle::Z || Session.mHandle == EHandle::XZ || Session.mHandle == EHandle::YZ) {
                Session.mScaleDelta.mZ = Delta;
            }
        }

        const FVector3 PrimaryScale{Session.mTargets.back().mRelative.GetScale()};
        FVector3 GroupFactors{Session.mUniformFactor, Session.mUniformFactor, Session.mUniformFactor};

        for (Uint32 Index{}; Index < 3; ++Index) {
            if (Session.mHandle != EHandle::Uniform && std::abs(PrimaryScale.V[Index]) > 0.000001f) {
                GroupFactors.V[Index] = LimitScale(PrimaryScale.V[Index], PrimaryScale.V[Index] + Session.mScaleDelta.V[Index]) / PrimaryScale.V[Index];
            }
        }

        for (const FTargetState& State : Session.mTargets) {
            FTransform Relative{State.mRelative};
            FVector3 Scale{State.mRelative.GetScale()};

            for (Uint32 Index{}; Index < 3; ++Index) {
                Scale.V[Index] = LimitScale(Scale.V[Index], Session.mHandle == EHandle::Uniform ? Scale.V[Index] * Session.mUniformFactor : Scale.V[Index] + Session.mScaleDelta.V[Index]);
            }

            Relative.SetScale(Scale);
            State.mTarget->SetRelativeTransform(Relative);
            const FVector3 Offset{State.mWorld.GetPosition() - Session.mPivot};
            FVector3 ScaledOffset{};

            for (Uint32 Index{}; Index < 3; ++Index) {
                ScaledOffset += Session.mAxes[Index] * Offset.Dot(Session.mAxes[Index]) * GroupFactors.V[Index];
            }

            State.mTarget->SetWorldLocation(Session.mPivot + ScaledOffset);
        }

        return;
    }

    FVector3 Delta{};
    FVector3 SurfaceNormal{};
    FVector3 SnappedPosition{};
    bool Snapped{};
    bool SurfaceAligned{};

    if (VertexSnap) {
        Snapped = FindVertex(Position, SnappedPosition);
    }

    if (!Snapped && SurfaceSnap && !Session.mPivotOnly) {
        Snapped = FindSurface(*Ray, SnappedPosition, SurfaceNormal);
        SurfaceAligned = Snapped;
        SnappedPosition += SurfaceNormal * Settings.mSurfaceSnapOffset;
    }

    if (Snapped) {
        Delta = SnappedPosition - Session.mPivot;
        mSnapPosition = SnappedPosition;
    } else if (Session.mScreenFallback) {
        const Uint32 AxisIndex{Session.mHandle >= EHandle::X && Session.mHandle <= EHandle::Z ? static_cast<Uint32>(Session.mHandle) - 1 : 0};
        Delta = Session.mAxes[AxisIndex] * ScreenDot(MouseDelta, Session.mScreenAxis) * Session.mUnitsPerPixel;
    } else {
        FVector3 Intersection{};

        if (!IntersectPlane(*Ray, Session.mPivot, Session.mPlaneNormal, Intersection)) {
            return;
        }

        const FVector3 Movement{Intersection - Session.mStartIntersection};

        if (Session.mHandle >= EHandle::X && Session.mHandle <= EHandle::Z) {
            const FVector3 Axis{Session.mAxes[static_cast<Uint32>(Session.mHandle) - 1]};
            Delta = Axis * SnapValue(Movement.Dot(Axis), Settings.mGridSize, Settings.mGridSnapEnabled);
        } else if (Session.mHandle == EHandle::Screen) {
            for (Uint32 Index{}; Index < 3; ++Index) {
                Delta.V[Index] = SnapValue(Movement.V[Index], Settings.mGridSize, Settings.mGridSnapEnabled);
            }
        } else {
            for (Uint32 Index{}; Index < 3; ++Index) {
                const bool Affected{Session.mHandle == EHandle::XY ? Index != 2 : Session.mHandle == EHandle::XZ ? Index != 1 : Index != 0};

                if (Affected) {
                    Delta += Session.mAxes[Index] * SnapValue(Movement.Dot(Session.mAxes[Index]), Settings.mGridSize, Settings.mGridSnapEnabled);
                }
            }
        }
    }

    if (!std::isfinite(Delta.mX) || !std::isfinite(Delta.mY) || !std::isfinite(Delta.mZ)) {
        return;
    }

    if (!Snapped) {
        mSnapPosition.reset();
    }

    Session.mTranslation = Delta;

    if (Session.mPivotOnly) {
        mEditorContext->SetGizmoPivot(Session.mPivot + Delta);
        return;
    }

    for (const FTargetState& State : Session.mTargets) {
        FTransform Desired{State.mWorld};
        Desired.SetPosition(State.mWorld.GetPosition() + Delta);

        if (SurfaceAligned && Settings.mSurfaceAlignRotation) {
            const FVector3 Up{State.mWorld.GetRotationQuaternion().RotateVector(FVector3::UnitZ)};
            Desired.SetRotation(AlignDirection(Up, SurfaceNormal) * State.mWorld.GetRotationQuaternion());
        }

        State.mTarget->SetWorldTransform(Desired);
    }

    if (Session.mHadPivot) {
        mEditorContext->SetGizmoPivot(Session.mPivot + Delta);
    }
}

void FTransformGizmo::EndDrag(bool Cancel) {
    if (!mDragSession.has_value()) {
        return;
    }

    FDragSession Session{std::move(*mDragSession)};
    mDragSession.reset();
    mEditorContext->SetGizmoInteracting(false);
    mHoveredHandle = EHandle::None;
    mSnapPosition.reset();
    const bool SelectionUnchanged{mEditorContext->GetSelectionRevision() == Session.mSelectionRevision};

    if (!Cancel || mEditorContext->GetWorld() != Session.mWorld) {
        return;
    }

    for (const FTargetState& State : Session.mTargets) {
        if (USceneComponent* Target{State.mTarget.Get()}; Target != nullptr && !Target->IsBeingDestroyed()) {
            Target->SetRelativeTransform(State.mRelative);
        }
    }

    if (!Session.mDuplicates.empty()) {
        for (const TObjectRef<AActor>& Entry : Session.mDuplicates) {
            if (AActor* Actor{Entry.Get()}) {
                Session.mWorld->DestroyActor(Actor);
            }
        }

        Session.mWorld->FlushPendingDestroyActors();
        TArray<AActor*> Originals{};

        for (const TObjectRef<AActor>& Entry : Session.mOriginalSelection) {
            if (AActor* Actor{Entry.Get()}) {
                Originals.push_back(Actor);
            }
        }

        if (SelectionUnchanged) {
            if (Originals.size() == 1 && Session.mOriginalComponent.Get() != nullptr) {
                mEditorContext->SetSelectedComponent(Session.mOriginalComponent.Get());
            } else {
                mEditorContext->SetSelectedActors(Originals);
            }
        }
    }

    if (!SelectionUnchanged) {
        return;
    }

    if (Session.mHadPivot) {
        mEditorContext->SetGizmoPivot(Session.mOriginalPivot);
    } else {
        mEditorContext->ResetGizmoPivot();
    }
}

bool FTransformGizmo::DuplicateSelection() {
    FDragSession& Session{*mDragSession};
    TArray<AActor*> Originals{};

    for (const TObjectRef<AActor>& Entry : Session.mOriginalSelection) {
        if (AActor* Actor{Entry.Get()}) {
            Originals.push_back(Actor);
        }
    }

    const TArray<AActor*> Duplicates{FWorldSerializer::DuplicateActors(*Session.mWorld, Originals)};

    if (Duplicates.size() != Originals.size()) {
        return false;
    }

    mEditorContext->SetSelectedActors(Duplicates);
    Session.mSelectionRevision = mEditorContext->GetSelectionRevision();
    Session.mTargets.clear();

    for (AActor* Actor : Duplicates) {
        Session.mDuplicates.push_back(TObjectRef<AActor>{Actor});
    }

    for (USceneComponent* Target : mEditorContext->GetSelectedTransformTargets()) {
        Session.mTargets.push_back({TObjectRef<USceneComponent>{Target}, Target->GetRelativeTransform(), Target->GetComponentTransform()});
    }

    if (Session.mHadPivot) {
        mEditorContext->SetGizmoPivot(Session.mPivot);
    }

    Session.mDuplicatePending = false;
    return !Session.mTargets.empty();
}

bool FTransformGizmo::IsSelectedHierarchy(const AActor* Actor) const {
    if (mEditorContext->IsActorSelected(Actor)) {
        return true;
    }

    for (const USceneComponent* Parent{Actor != nullptr && Actor->GetRootComponent() != nullptr ? Actor->GetRootComponent()->GetParent() : nullptr}; Parent != nullptr; Parent = Parent->GetParent()) {
        if (mEditorContext->IsActorSelected(Parent->GetOwner())) {
            return true;
        }
    }

    return false;
}

bool FTransformGizmo::FindSurface(const FRay& Ray, FVector3& Position, FVector3& Normal) const {
    UWorld* World{mEditorContext->GetWorld()};
    const UMeshComponent* Nearest{};
    float NearestDistance{std::numeric_limits<float>::max()};

    if (World == nullptr) {
        return false;
    }

    for (const std::unique_ptr<AActor>& Actor : World->GetActors()) {
        if (Actor->IsBeingDestroyed() || IsSelectedHierarchy(Actor.get())) {
            continue;
        }

        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            if (!Component->IsA(UMeshComponent::StaticClass())) {
                continue;
            }

            const UMeshComponent* Mesh{static_cast<const UMeshComponent*>(Component.get())};
            float Distance{};

            if (Mesh->IsRegistered() && Mesh->IsVisible() && Mesh->RaycastMesh(Ray, Distance, NearestDistance) && Distance < NearestDistance) {
                Nearest = Mesh;
                NearestDistance = Distance;
            }
        }
    }

    if (Nearest == nullptr) {
        return false;
    }

    Position = FVector3{Ray.position} + FVector3{Ray.direction} * NearestDistance;
    Normal = -FVector3{Ray.direction};
    const UMesh* Mesh{mAssetRegistry->ResolveAsset<UMesh>(Nearest->GetMeshHandle())};

    if (Mesh != nullptr) {
        const auto Positions{Mesh->GetVertexAttributeData<EVertexAttribute::Position>()};
        const auto& Indices{Mesh->GetIndices()};
        const FMatrix WorldTransform{Nearest->GetComponentToWorld()};

        for (std::size_t Index{}; Index + 2 < Indices.size(); Index += 3) {
            if (Indices[Index] >= Positions.size() || Indices[Index + 1] >= Positions.size() || Indices[Index + 2] >= Positions.size()) {
                continue;
            }

            const FVector3 First{FVector3::Transform(Positions[Indices[Index]], WorldTransform)};
            const FVector3 Second{FVector3::Transform(Positions[Indices[Index + 1]], WorldTransform)};
            const FVector3 Third{FVector3::Transform(Positions[Indices[Index + 2]], WorldTransform)};
            float Distance{};

            if (Ray.Intersects(First.ToSimpleMath(), Second.ToSimpleMath(), Third.ToSimpleMath(), Distance) && std::abs(Distance - NearestDistance) < std::max(0.001f, NearestDistance * 0.0001f)) {
                Normal = (Second - First).Cross(Third - First);
                Normal.Normalize();

                if (Normal.Dot(FVector3{Ray.direction}) > 0.0f) {
                    Normal = -Normal;
                }

                break;
            }
        }
    }

    return true;
}

bool FTransformGizmo::FindVertex(const POINT& MousePosition, FVector3& Position) const {
    UWorld* World{mEditorContext->GetWorld()};
    const FViewMatrices& Camera{mDragSession.has_value() ? mDragSession->mCamera : mLastCamera};
    const D3D11_VIEWPORT& Viewport{mDragSession.has_value() ? mDragSession->mViewport : mLastViewport};
    const FVector2 Mouse{static_cast<float>(MousePosition.x), static_cast<float>(MousePosition.y)};
    float BestDistance{16.0f};
    float BestDepth{std::numeric_limits<float>::max()};
    bool Found{};

    if (World == nullptr || mAssetRegistry == nullptr) {
        return false;
    }

    for (const std::unique_ptr<AActor>& Actor : World->GetActors()) {
        if (Actor->IsBeingDestroyed() || (IsSelectedHierarchy(Actor.get()) && (!mDragSession.has_value() || !mDragSession->mPivotOnly))) {
            continue;
        }

        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            if (!Component->IsA(UMeshComponent::StaticClass())) {
                continue;
            }

            const UMeshComponent* MeshComponent{static_cast<const UMeshComponent*>(Component.get())};
            const UMesh* Mesh{MeshComponent->IsRegistered() && MeshComponent->IsVisible() ? mAssetRegistry->ResolveAsset<UMesh>(MeshComponent->GetMeshHandle()) : nullptr};

            if (Mesh == nullptr) {
                continue;
            }

            std::array<DirectX::XMFLOAT3, 8> Corners{};
            MeshComponent->GetWorldOBB().GetCorners(Corners.data());
            FVector2 Minimum{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
            FVector2 Maximum{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
            bool AllProjected{true};

            for (const DirectX::XMFLOAT3& Corner : Corners) {
                FVector2 Screen{};

                if (!ProjectPoint(FVector3{Corner}, Camera, Viewport, Screen)) {
                    AllProjected = false;
                    break;
                }

                Minimum.mX = std::min(Minimum.mX, Screen.mX);
                Minimum.mY = std::min(Minimum.mY, Screen.mY);
                Maximum.mX = std::max(Maximum.mX, Screen.mX);
                Maximum.mY = std::max(Maximum.mY, Screen.mY);
            }

            if (AllProjected && (Mouse.mX < Minimum.mX - 16.0f || Mouse.mY < Minimum.mY - 16.0f || Mouse.mX > Maximum.mX + 16.0f || Mouse.mY > Maximum.mY + 16.0f)) {
                continue;
            }

            const FMatrix WorldTransform{MeshComponent->GetComponentToWorld()};

            for (const FVector3& Vertex : Mesh->GetVertexAttributeData<EVertexAttribute::Position>()) {
                const FVector3 WorldVertex{FVector3::Transform(Vertex, WorldTransform)};
                FVector2 Screen{};

                if (!ProjectPoint(WorldVertex, Camera, Viewport, Screen)) {
                    continue;
                }

                const float Distance{(Mouse - Screen).Length()};
                const float Depth{FVector3::Transform(WorldVertex, Camera.mView).mZ};

                if (Depth >= 0.0f && (Distance < BestDistance - 0.001f || (std::abs(Distance - BestDistance) < 0.001f && Depth < BestDepth))) {
                    BestDistance = Distance;
                    BestDepth = Depth;
                    Position = WorldVertex;
                    Found = true;
                }
            }
        }
    }

    return Found;
}

FVector4 FTransformGizmo::GetHandleColor(EHandle Handle) const {
    const EHandle Highlight{mDragSession.has_value() ? mDragSession->mHandle : mHoveredHandle};

    if (Handle == Highlight) {
        return {1.0f, 0.85f, 0.05f, 1.0f};
    }

    FVector4 Color{0.8f, 0.8f, 0.8f, 1.0f};

    switch (Handle) {
        case EHandle::X:
        case EHandle::YZ: Color = {0.95f, 0.12f, 0.12f, 1.0f}; break;
        case EHandle::Y:
        case EHandle::XZ: Color = {0.15f, 0.9f, 0.2f, 1.0f}; break;
        case EHandle::Z:
        case EHandle::XY: Color = {0.15f, 0.4f, 1.0f, 1.0f}; break;
        default: break;
    }

    if (mDragSession.has_value()) {
        Color.mX *= 0.4f;
        Color.mY *= 0.4f;
        Color.mZ *= 0.4f;
    }

    return Color;
}

FAssetHandle FTransformGizmo::GetHandleMaterial(EHandle Handle) const {
    const EHandle Highlight{mDragSession.has_value() ? mDragSession->mHandle : mHoveredHandle};

    if (Handle == Highlight && mHighlightMaterial) {
        return mHighlightMaterial;
    }

    if (Handle >= EHandle::X && Handle <= EHandle::Z) {
        return mAxisMaterials[static_cast<Uint32>(Handle) - 1];
    }

    return mNeutralMaterial;
}

FVector3 FTransformGizmo::GetHandleAxis(EHandle Handle) const {
    return Handle >= EHandle::X && Handle <= EHandle::Z ? mAxes[static_cast<Uint32>(Handle) - 1] : mViewDirection;
}

void FTransformGizmo::BuildGizmoDrawData(FOverlayRenderData& Overlay) {
    if (!mVisible || !Overlay.IsPassEnabled(EOverlayPass::Gizmo)) {
        return;
    }

    Overlay.mGizmoMeshes.insert(Overlay.mGizmoMeshes.end(), mMeshes.begin(), mMeshes.end());

    for (const FLineDrawData& Line : mLines.GetLines()) {
        Overlay.mGizmoLines.AddLine(Line.mStart, Line.mEnd, Line.mColor, Line.mWidthPixels, ELineDepthMode::Overlay);
    }

    BuildFeedback(Overlay);
}

void FTransformGizmo::BuildFeedback(FOverlayRenderData& Overlay) const {
    if (mSnapPosition.has_value()) {
        const float Radius{5.0f * mUnitsPerPixel};
        const FVector3 Vertex{*mSnapPosition};
        Overlay.mGizmoLines.AddLine(Vertex - mViewRight * Radius, Vertex + mViewRight * Radius, {1.0f, 0.85f, 0.05f, 1.0f}, 3.0f, ELineDepthMode::Overlay);
        Overlay.mGizmoLines.AddLine(Vertex - mViewUp * Radius, Vertex + mViewUp * Radius, {1.0f, 0.85f, 0.05f, 1.0f}, 3.0f, ELineDepthMode::Overlay);
    }

    if (!mDragSession.has_value()) {
        return;
    }

    const FDragSession& Session{*mDragSession};
    const FVector4 Color{1.0f, 0.85f, 0.05f, 1.0f};
    char Text[160]{};

    if (Session.mMode == EGizmoMode::Rotate) {
        const float Radius{RingRadius * mUnitsPerPixel * mSize};
        const FVector3 Start{Session.mStartDirection};
        const FVector3 End{FQuat::CreateFromAxisAngle(Session.mPlaneNormal, Session.mAppliedAngle).RotateVector(Start)};
        Overlay.mGizmoLines.AddLine(Session.mPivot, Session.mPivot + Start * Radius, {0.8f, 0.8f, 0.8f, 1.0f}, 1.5f, ELineDepthMode::Overlay);
        Overlay.mGizmoLines.AddLine(Session.mPivot, Session.mPivot + End * Radius, Color, 2.0f, ELineDepthMode::Overlay);
        const float DisplayAngle{std::clamp(Session.mAppliedAngle, -2.0f * Pi, 2.0f * Pi)};
        const Uint32 Segments{static_cast<Uint32>(std::max(1.0f, std::ceil(std::abs(DisplayAngle) * 18.0f)))};

        for (Uint32 Index{}; Index < Segments; ++Index) {
            const float FirstAngle{DisplayAngle * static_cast<float>(Index) / Segments};
            const float SecondAngle{DisplayAngle * static_cast<float>(Index + 1) / Segments};
            const FVector3 First{Session.mPivot + FQuat::CreateFromAxisAngle(Session.mPlaneNormal, FirstAngle).RotateVector(Start) * Radius};
            const FVector3 Second{Session.mPivot + FQuat::CreateFromAxisAngle(Session.mPlaneNormal, SecondAngle).RotateVector(Start) * Radius};
            Overlay.mGizmoLines.AddLine(First, Second, Color, 4.0f, ELineDepthMode::Overlay);
            Overlay.mGizmoLines.AddLine(Session.mPivot, Second, {1.0f, 0.85f, 0.05f, 0.15f}, 1.0f, ELineDepthMode::Overlay);
        }

        std::snprintf(Text, sizeof(Text), "%+.2f deg", Session.mAppliedAngle * 180.0f / Pi);
    } else if (Session.mMode == EGizmoMode::Scale) {
        std::snprintf(Text, sizeof(Text), "Scale %+.3f / %+.3f / %+.3f | x%.3f", Session.mScaleDelta.mX, Session.mScaleDelta.mY, Session.mScaleDelta.mZ, Session.mUniformFactor);
    } else {
        Overlay.mGizmoLines.AddLine(Session.mPivot, Session.mPivot + Session.mTranslation, Color, 1.5f, ELineDepthMode::Overlay);
        std::snprintf(Text, sizeof(Text), "%s %+.2f / %+.2f / %+.2f", Session.mPivotOnly ? "Pivot" : "Move", Session.mTranslation.mX, Session.mTranslation.mY, Session.mTranslation.mZ);
    }

    FOverlayTextDrawData Label{};
    Label.mWorldAnchor = mPivot;
    Label.mScreenOffset = {14.0f, 22.0f};
    Label.mText = Text;
    Label.mColor = Color;
    Label.mPixelHeight = 16.0f;
    Overlay.mTextDraws.push_back(std::move(Label));
}

void FTransformGizmo::RefreshAssetHandles() {
    if (mAssetRegistry == nullptr) {
        return;
    }

    mArrowMesh = mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Mesh/GizmoArrow.bin"});
    mScaleArrowMesh = mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Mesh/GizmoScaleArrow.bin"});
    mCubeMesh = mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Mesh/Cube.bin"});
    mAxisMaterials = {mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Red.mtl"}), mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Green.mtl"}), mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/Blue.mtl"})};
    mHighlightMaterial = mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/GizmoHighlight.mtl"});
    mNeutralMaterial = mAssetRegistry->FindAsset(FAssetPath{"/Game/System/Material/GizmoNeutral.mtl"});
    mGizmoPipeline = mAssetRegistry->FindAsset(FAssetPath{"/Game/Pipeline/Gizmo.json"});
}
