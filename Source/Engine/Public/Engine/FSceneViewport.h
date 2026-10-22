#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class IRendererModule;
class FViewportClient;
class FRenderTarget;
struct FViewportInput;
struct FVector2;

class FSceneViewport {
public:
    ReflectTypes(FSceneViewport);

public:
    FSceneViewport(IRendererModule& Renderer, FViewportClient& Client);
    ~FSceneViewport();

public:
    ReflectStruct(FSceneViewport);

    bool Resize(Uint32 Width, Uint32 Height);

    void Input(const FViewportInput& Input, float DeltaTime);
    void Draw();

    FRenderTarget* GetRenderTarget() const;
    FVector2 GetSize() const;

private:
    ReflectMembers();

private:
    IRendererModule* mRenderer{};
    ReflectProperty(mRenderer, "Renderer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FViewportClient* mClient{};
    ReflectProperty(mClient, "Client", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::unique_ptr<FRenderTarget> mTarget{};
    ReflectProperty(mTarget, "Target", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
