#pragma once
#include <d3d11.h>

struct RenderWindowInfo {
    UINT mScreenWidth{0};
    UINT mScreenHeight{0};
    D3D11_VIEWPORT mViewport{};
};
