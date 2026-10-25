#pragma once

#include "Engine/FViewportClient.h"
#include "Engine/FWorldHandle.h"
#include "Engine/FSceneView.h"

class UEngine;

class FGameViewportClient final : public FViewportClient {
public:
    ReflectTypes(FGameViewportClient, FViewportClient);

public:
    explicit FGameViewportClient(UEngine* Engine);

public:
    ReflectStruct(FGameViewportClient);
    bool SetWorld(FWorldHandle World);
    FWorldHandle GetWorldHandle() const;
    bool CalcSceneView(const FVector2& Size, FSceneView& View) const;
    void Input(FSceneViewport* Viewport, const FViewportInput& Input, float DeltaTime) override;
    void Draw(FSceneViewport* Viewport) override;

private:
    ReflectMembers();

private:
    UEngine* mEngine{};
    ReflectProperty(mEngine, "Engine", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FWorldHandle mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
