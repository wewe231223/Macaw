#pragma once
#include "World/UWorld.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

class FAssetRegistry;
class FRenderer;
class FRenderScene;
class UDirectionalLightComponent;

class FPreviewScene final {
public:
    FPreviewScene() = default;
    ~FPreviewScene();
    FPreviewScene(const FPreviewScene&) = delete;
    FPreviewScene& operator=(const FPreviewScene&) = delete;
    FPreviewScene(FPreviewScene&&) = delete;
    FPreviewScene& operator=(FPreviewScene&&) = delete;

public:
    bool Initialize(FRenderer& Renderer, FAssetRegistry& Registry);
    void SetMesh(const FMeshSceneData& Mesh, const FMatrix& World);
    void SetLight(const FVector3& Direction, const FVector3& Color, float Intensity);
    const FRenderScene* Synchronize();
    void Reset();

private:
    UWorld mWorld{};
    FRenderer* mRenderer{nullptr};
    const FAssetRegistry* mRegistry{nullptr};
    UStaticMeshComponent* mMesh{nullptr};
    UDirectionalLightComponent* mLight{nullptr};
};
