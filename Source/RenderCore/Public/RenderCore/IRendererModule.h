#pragma once

#include "Core/ReflectionMacros.h"
#include "RenderCore/FRenderStats.h"

class FSceneInterface;
class FRenderTarget;
struct FSceneViewFamily;
struct FMeshData;
enum class EPrimitiveType : Uint8;

class IRendererModule {
public:
    ReflectTypes(IRendererModule);

public:
    virtual ~IRendererModule();

public:
    ReflectStruct(IRendererModule);

    virtual bool Initialize(void* WindowHandle, Uint32 Width, Uint32 Height) = 0;
    virtual void Shutdown() = 0;

    virtual std::unique_ptr<FSceneInterface> AllocateScene() = 0;
    virtual bool UploadMesh(EPrimitiveType Type, const FMeshData& Mesh) = 0;

    virtual std::unique_ptr<FRenderTarget> CreateRenderTarget(Uint32 Width, Uint32 Height) = 0;
    virtual bool ResizeRenderTarget(FRenderTarget& Target, Uint32 Width, Uint32 Height) = 0;

    virtual void BeginFrame(Uint32 Width, Uint32 Height) = 0;
    virtual void BeginRenderingViewFamily(const FSceneViewFamily& Family) = 0;

    virtual void BeginUiRender() = 0;
    virtual void Present() = 0;

    virtual void* GetNativeDevice() const = 0;
    virtual void* GetNativeContext() const = 0;
    virtual FRenderStats GetStats() const = 0;

private:
    ReflectMembers();
};
