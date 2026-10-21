#pragma once

#include "Engine/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/FRenderStats.h"
#include "RenderCore/IRendererModule.h"

class FRenderer final : public IRendererModule {
private:
    struct FImplementation;

public:
    FRenderer();
    ~FRenderer() override;

public:
    bool Initialize(void* WindowHandle, Uint32 Width, Uint32 Height) override;
    void Shutdown() override;

    std::unique_ptr<FSceneInterface> AllocateScene() override;
    bool UploadMesh(EPrimitiveType Type, const FMeshData& Mesh) override;

    std::unique_ptr<FRenderTarget> CreateRenderTarget(Uint32 Width, Uint32 Height) override;
    bool ResizeRenderTarget(FRenderTarget& Target, Uint32 Width, Uint32 Height) override;

    void BeginFrame(Uint32 Width, Uint32 Height) override;
    void BeginRenderingViewFamily(const FSceneViewFamily& Family) override;

    void BeginUiRender() override;
    void Present() override;

    void* GetNativeDevice() const override;
    void* GetNativeContext() const override;
    FRenderStats GetStats() const override;

private:
    std::unique_ptr<FImplementation> mImplementation{};
};
