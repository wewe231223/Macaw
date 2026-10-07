#pragma once

#include <d3d11.h>

class IRenderSurface;

struct FSceneRenderOutput {
    bool IsValid() const;

    const IRenderSurface* mTarget{nullptr};
    ID3D11ShaderResourceView* mColorResource{nullptr};
    ID3D11ShaderResourceView* mDepthResource{nullptr};
    ID3D11DepthStencilView* mDepthStencilView{nullptr};
    D3D11_VIEWPORT mViewport{};
};
