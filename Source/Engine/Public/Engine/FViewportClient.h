#pragma once

#include "Core/Types.h"

class FSceneViewport;
struct FViewportInput;
struct FVector2;

class FViewportClient {
public:
    virtual ~FViewportClient();

public:
    virtual void Input(const FViewportInput& Input, const FVector2& Size, float DeltaTime) = 0;
    virtual void Draw(FSceneViewport& Viewport) = 0;
};
