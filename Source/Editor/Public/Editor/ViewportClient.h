#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FViewportClient.h"
#include "Editor/Gizmo.h"

class UEditorEngine;

class FEditorViewportClient final : public FViewportClient {
public:
    ReflectTypes(FEditorViewportClient, FViewportClient);

public:
    explicit FEditorViewportClient(UEditorEngine& Engine);
    ~FEditorViewportClient() override;

public:
    ReflectStruct(FEditorViewportClient);

    void Input(const FViewportInput& Input, const FVector2& Size, float DeltaTime) override;
    void Draw(FSceneViewport& Viewport) override;

    FSceneView CalcSceneView(const FVector2& Size) const;

    void ResetCamera();
    void SetCamera(const FVector& Location, const FRotator& Rotation);
    const FVector& GetCameraLocation() const;

    FTransformGizmo& GetGizmo();

private:
    bool NavigateCamera(const FViewportInput& Input, const FVector2& Size, float DeltaTime);
    void FocusSelection(const FVector2& Size);

private:
    ReflectMembers();

private:
    UEditorEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FVector mCameraLocation{-7.0f, -9.0f, 6.0f};
    ReflectProperty(mCameraLocation, "CameraLocation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FRotator mCameraRotation{-26.0f, 50.0f, 0.0f};
    ReflectProperty(mCameraRotation, "CameraRotation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector mOrbitPivot{};
    ReflectProperty(mOrbitPivot, "OrbitPivot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mCameraSpeed{4.0f};
    ReflectProperty(mCameraSpeed, "CameraSpeed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FTransformGizmo mGizmo;
    ReflectProperty(mGizmo, "Gizmo", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FPrimitiveDrawInterface mDrawInterface{};
    ReflectProperty(mDrawInterface, "DrawInterface", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
