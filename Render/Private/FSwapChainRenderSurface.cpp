#include "pch.h"
#include "Render/FSwapChainRenderSurface.h"
#include "Core/Base/ErrorHandler.h"
#include <utility>

void FSwapChainRenderSurface::Initialize(ID3D11Device* Device, IDXGISwapChain* InSwapChain) {
    Reset();

    if (Device == nullptr || InSwapChain == nullptr) {
        return;
    }

    mSwapChain = InSwapChain;
    CreateResources(Device);
}

bool FSwapChainRenderSurface::Resize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
    if (Device == nullptr || mSwapChain == nullptr || Width == 0 || Height == 0) {
        return false;
    }

    FSceneRenderSurface::Reset();

    DXGI_SWAP_CHAIN_DESC SwapChainDescription{};

    ErrorHandler::ReportHRESULT(mSwapChain->GetDesc(&SwapChainDescription), "[ FSwapChainRenderSurface ]", "Failed to get swap chain description.", ErrorHandler::EErrorLevel::Critical);
    ErrorHandler::ReportHRESULT(mSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, SwapChainDescription.Flags), "[ FSwapChainRenderSurface ]", "Failed to resize swap chain buffers.", ErrorHandler::EErrorLevel::Critical);
    CreateResources(Device);

    return IsValid();
}

void FSwapChainRenderSurface::Reset() {
    FSceneRenderSurface::Reset();
    mSwapChain.Reset();
}

void FSwapChainRenderSurface::CreateResources(ID3D11Device* Device) {
    Microsoft::WRL::ComPtr<ID3D11Texture2D> ColorTexture{};
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView{};

    ErrorHandler::ReportHRESULT(mSwapChain->GetBuffer(0, IID_PPV_ARGS(ColorTexture.GetAddressOf())), "[ FSwapChainRenderSurface ]", "Failed to get swap chain back buffer.", ErrorHandler::EErrorLevel::Critical);
    ErrorHandler::ReportHRESULT(Device->CreateRenderTargetView(ColorTexture.Get(), nullptr, RenderTargetView.GetAddressOf()), "[ FSwapChainRenderSurface ]", "Failed to create swap chain render target view.", ErrorHandler::EErrorLevel::Critical);
    InitializeResources(Device, std::move(ColorTexture), std::move(RenderTargetView), {});
}
