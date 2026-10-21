#pragma once

#include "Engine/FPrimitiveSceneProxy.h"

#include <d3d11.h>
#include <wrl/client.h>

struct FScenePrimitive {
    FPrimitiveSceneProxy mProxy{};
    Microsoft::WRL::ComPtr<ID3D11Buffer> mObjectBuffer{};
};
