#pragma once

#include "Render/FSceneRenderSurface.h"

class FSwapChainRenderSurface final : public FSceneRenderSurface {
public:
    FSwapChainRenderSurface() = default;
    ~FSwapChainRenderSurface() override = default;

    FSwapChainRenderSurface(const FSwapChainRenderSurface&) = delete;
    FSwapChainRenderSurface& operator=(const FSwapChainRenderSurface&) = delete;
    FSwapChainRenderSurface(FSwapChainRenderSurface&&) = delete;
    FSwapChainRenderSurface& operator=(FSwapChainRenderSurface&&) = delete;

public:
    void Initialize(ID3D11Device* Device, IDXGISwapChain* SwapChain);
    bool Resize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) override;
    void Reset() override;

private:
    void CreateResources(ID3D11Device* Device);

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
};
