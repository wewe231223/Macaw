/**
 * @file FRenderContext.h
 * @brief 렌더 호출에 귀속되는 장치와 GPU 자원만 전달하는 컨텍스트를 정의합니다.
 */
#pragma once

#include <d3d11.h>
#include "Core/Common.h"

class FRenderAssetResources;
class FRenderBuffers;

struct FRenderContext {
    ID3D11DeviceContext* mDeviceContext{nullptr};
    FRenderAssetResources* mAssetResources{nullptr};
    ID3D11ShaderResourceView* mMaterialResource{nullptr};
    FRenderBuffers* mRenderBuffers{nullptr};
};
