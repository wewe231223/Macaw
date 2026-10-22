#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class FSceneViewport;
struct FViewportInput;
struct FVector2;

class FViewportClient {
public:
    ReflectTypes(FViewportClient);

public:
    virtual ~FViewportClient();

public:
    ReflectStruct(FViewportClient);

    virtual void Input(const FViewportInput& Input, const FVector2& Size, float DeltaTime) = 0;
    virtual void Draw(FSceneViewport& Viewport) = 0;

private:
    ReflectMembers();
};
