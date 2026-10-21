#pragma once

#include "Core/Types.h"

class IRendererModule;
class FViewportClient;
class FRenderTarget;
struct FViewportInput;
struct FVector2;

class FSceneViewport {
public:
    FSceneViewport(IRendererModule& Renderer, FViewportClient& Client);
    ~FSceneViewport();

public:
    bool Resize(Uint32 Width, Uint32 Height);

    void Input(const FViewportInput& Input, float DeltaTime);
    void Draw();

    FRenderTarget* GetRenderTarget() const;
    FVector2 GetSize() const;

private:
    IRendererModule* mRenderer{};
    FViewportClient* mClient{};
    std::unique_ptr<FRenderTarget> mTarget{};
};
