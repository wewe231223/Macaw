#pragma once

#include <d3d11.h>

class FSceneRenderSurface;

struct FSceneRenderOutput {
    bool IsValid() const;

    const FSceneRenderSurface* mTarget{nullptr};
    ID3D11ShaderResourceView* mColorResource{nullptr};
    ID3D11ShaderResourceView* mDepthResource{nullptr};
    ID3D11DepthStencilView* mDepthStencilView{nullptr};
    D3D11_VIEWPORT mViewport{};
};
