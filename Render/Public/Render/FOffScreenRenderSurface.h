#pragma once

#include "Render/FSceneRenderSurface.h"

class FOffScreenRenderSurface final : public FSceneRenderSurface {
public:
    FOffScreenRenderSurface() = default;
    ~FOffScreenRenderSurface() override = default;

    FOffScreenRenderSurface(const FOffScreenRenderSurface&) = delete;
    FOffScreenRenderSurface& operator=(const FOffScreenRenderSurface&) = delete;
    FOffScreenRenderSurface(FOffScreenRenderSurface&&) = delete;
    FOffScreenRenderSurface& operator=(FOffScreenRenderSurface&&) = delete;

public:
    void Initialize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height, DXGI_FORMAT ColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
    bool Resize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) override;
    void Reset() override;

private:
    void CreateResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height);

private:
    DXGI_FORMAT mColorFormat{DXGI_FORMAT_R8G8B8A8_UNORM};
};
