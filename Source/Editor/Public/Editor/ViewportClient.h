#pragma once

#include "Engine/FViewportClient.h"
#include "Editor/Gizmo.h"

class UEditorEngine;

class FEditorViewportClient final : public FViewportClient {
public:
    explicit FEditorViewportClient(UEditorEngine& Engine);
    ~FEditorViewportClient() override;

public:
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
    UEditorEngine* mEngine{};

    FVector mCameraLocation{-7.0f, -9.0f, 6.0f};
    FRotator mCameraRotation{-26.0f, 50.0f, 0.0f};
    FVector mOrbitPivot{};
    float mCameraSpeed{4.0f};

    FTransformGizmo mGizmo;
    FPrimitiveDrawInterface mDrawInterface{};
};
