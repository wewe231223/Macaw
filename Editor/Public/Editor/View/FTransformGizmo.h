/**
 * @file FTransformGizmo.h
 * @brief 월드별 편집 상태와 전역 에셋 및 설정을 사용하는 뷰포트 도구를 정의합니다.
 */
#pragma once

#include <array>
#include <optional>
#include <d3d11.h>
#include "Core/Base/FAssetHandle.h"
#include "CoreUObject/TObjectRef.h"
#include "Editor/Input/FKeyboardInput.h"
#include "Editor/Input/FMouseInput.h"
#include "Editor/World/FWorldEditorContext.h"
#include "RenderCore/FOverlayRenderData.h"
#include "World/Component/USceneComponent.h"

class FTransformGizmo {
private:
    enum class EHandle : Uint8 {
        None,
        X,
        Y,
        Z,
        XY,
        XZ,
        YZ,
        Screen,
        Uniform
    };

    struct FPickShape {
        EHandle mHandle{EHandle::None};
        std::array<FVector3, 4> mPoints{};
        Uint32 mCount{};
        float mTolerance{8.0f};
        Uint32 mPriority{2};
    };

    struct FTargetState {
        TObjectRef<USceneComponent> mTarget{};
        FTransform mRelative{};
        FTransform mWorld{};
    };

    struct FDragSession {
        TArray<FTargetState> mTargets{};
        TArray<TObjectRef<AActor>> mOriginalSelection{};
        TArray<TObjectRef<AActor>> mDuplicates{};
        TObjectRef<UActorComponent> mOriginalComponent{};
        UWorld* mWorld{};
        FViewMatrices mCamera{};
        D3D11_VIEWPORT mViewport{};
        FMatrix mBasis{FMatrix::Identity};
        std::array<FVector3, 3> mAxes{};
        FVector3 mPivot{};
        FVector3 mPlaneNormal{};
        FVector3 mStartIntersection{};
        FVector3 mStartDirection{};
        FVector3 mPreviousDirection{};
        FVector3 mTranslation{};
        FVector3 mScaleDelta{};
        FVector3 mOriginalPivot{};
        POINT mStartMouse{};
        FVector2 mScreenAxis{};
        float mUnitsPerPixel{1.0f};
        float mRawAngle{};
        float mAppliedAngle{};
        float mUniformFactor{1.0f};
        Uint64 mSelectionRevision{};
        EHandle mHandle{EHandle::None};
        EGizmoMode mMode{EGizmoMode::Translate};
        EMouseSide mButton{Left};
        bool mPivotOnly{};
        bool mHadPivot{};
        bool mScreenFallback{};
        bool mDuplicatePending{};
    };

public:
    FTransformGizmo() = default;
    ~FTransformGizmo() = default;
    FTransformGizmo(const FTransformGizmo&) = delete;
    FTransformGizmo& operator=(const FTransformGizmo&) = delete;
    FTransformGizmo(FTransformGizmo&&) = default;
    FTransformGizmo& operator=(FTransformGizmo&&) = default;

public:
    /**
     * @brief 대상 월드의 편집 컨텍스트에 연결하고 에셋과 설정은 전역 서비스에서 조회합니다.
     * @param Device 렌더링에 사용할 그래픽 장치입니다.
     * @param InEditorContext 선택과 조작 상태를 보관하는 월드별 컨텍스트입니다.
     */
    void Initialize(ID3D11Device* Device, FWorldEditorContext& InEditorContext);
    void ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, bool BMouseCapturedByUi, bool BKeyboardCapturedByUi);
    void Update(const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport);
    void BuildGizmoDrawData(FOverlayRenderData& Overlay);

private:
    void BuildHandles();
    void BuildAxes();
    void BuildPlanes();
    void BuildRotation();
    void BuildFeedback(FOverlayRenderData& Overlay) const;
    void AddSegment(EHandle Handle, const FVector3& Start, const FVector3& End, const FVector4& Color, float Width, bool Pickable = true);
    void AddPolygon(EHandle Handle, const std::array<FVector3, 4>& Points, Uint32 Count, const FVector4& Color, Uint32 Priority);
    EHandle HitTest(const POINT& Position) const;
    bool BeginDrag(EHandle Handle, const POINT& Position, EMouseSide Button, bool PivotOnly, bool Duplicate);
    void UpdateDrag(const POINT& Position, bool VertexSnap, bool SurfaceSnap);
    void EndDrag(bool Cancel);
    bool DuplicateSelection();
    bool FindSurface(const FRay& Ray, FVector3& Position, FVector3& Normal) const;
    bool FindVertex(const POINT& MousePosition, FVector3& Position) const;
    bool IsSelectedHierarchy(const AActor* Actor) const;
    FVector4 GetHandleColor(EHandle Handle) const;
    FAssetHandle GetHandleMaterial(EHandle Handle) const;
    FVector3 GetHandleAxis(EHandle Handle) const;
    void RefreshAssetHandles();

private:
    FWorldEditorContext* mEditorContext{};
    FAssetHandle mArrowMesh{};
    FAssetHandle mScaleArrowMesh{};
    FAssetHandle mCubeMesh{};
    std::array<FAssetHandle, 3> mAxisMaterials{};
    FAssetHandle mHighlightMaterial{};
    FAssetHandle mNeutralMaterial{};
    FAssetHandle mGizmoPipeline{};
    FViewMatrices mLastCamera{};
    D3D11_VIEWPORT mLastViewport{};
    FMatrix mBasis{FMatrix::Identity};
    std::array<FVector3, 3> mAxes{FVector3::UnitX, FVector3::UnitY, FVector3::UnitZ};
    FVector3 mPivot{};
    FVector3 mViewDirection{};
    FVector3 mViewRight{};
    FVector3 mViewUp{};
    float mUnitsPerPixel{1.0f};
    float mSize{1.0f};
    TArray<FPickShape> mPickShapes{};
    TArray<FOverlayMeshDrawData> mMeshes{};
    FLineRenderData mLines{};
    std::optional<FDragSession> mDragSession{};
    EHandle mHoveredHandle{EHandle::None};
    bool mVisible{};
    bool mHasCamera{};
    bool mVertexSnap{};
    std::optional<FVector3> mSnapPosition{};
};
