#pragma once
#include "RenderCore/FRenderData.h"
#include "CoreUObject/FObjectHandle.h"

class FLightSceneProxy {
public:
    FLightSceneProxy(FObjectHandle ComponentHandle, const FLightShaderParameters& Parameters);

public:
    FObjectHandle GetComponentHandle() const;
    const FLightShaderParameters& GetShaderParameters() const;
    void SetTransform(const FMatrix& World);

private:
    FObjectHandle mComponentHandle{};
    FLightShaderParameters mParameters{};
};
