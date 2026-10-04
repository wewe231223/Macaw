#pragma once

#include <d3d11.h>
#include "Core/Common.h"

class IAssetRegistry;
class FRenderAssetResources;
class FFrameResource;

struct FRenderContext {
    ID3D11DeviceContext* mDeviceContext{nullptr};
    const IAssetRegistry* mAssetRegistry{nullptr};
    FRenderAssetResources* mAssetResources{nullptr};
    ID3D11ShaderResourceView* mMaterialResource{nullptr};
    FFrameResource* mFrameResource{nullptr};
};
