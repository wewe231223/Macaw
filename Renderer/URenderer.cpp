#include "pch.h"
#include "URenderer.h"

#pragma comment(lib, "d3d11.lib")

bool URenderer::Create(HWND hWnd)
{
    if (!CreateDeviceAndSwapChain(hWnd))
    {
        return false; 
    }

    if (!CreateFrameBuffer())
    {
        return false;
    }

    return true;
}

void URenderer::Prepare()
{
    const float ClearColor[4] =
    {
        0.1f, 0.1f, 0.1f, 1.0f
    };

    DeviceContext->OMSetRenderTargets(
        1,
        &FrameBufferRTV,
        nullptr
    );

    DeviceContext->ClearRenderTargetView(
        FrameBufferRTV,
        ClearColor
    );
}

void URenderer::Present()
{
    SwapChain->Present(1, 0);
}

void URenderer::Release()
{
    if (DeviceContext)
    {
        DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    }

    if (FrameBufferRTV)
    {
        FrameBufferRTV->Release();
        FrameBufferRTV = nullptr;
    }

    if (FrameBuffer)
    {
        FrameBuffer->Release();
        FrameBuffer = nullptr;
    }

    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }

    if (DeviceContext)
    {
        DeviceContext->Release();
        DeviceContext = nullptr;
    }

    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }
}

bool URenderer::CreateDeviceAndSwapChain(HWND hWnd)
{
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0
    };

    DXGI_SWAP_CHAIN_DESC Desc = {};

    Desc.BufferDesc.Width = 0;
    Desc.BufferDesc.Height = 0;
    Desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    Desc.SampleDesc.Count = 1;

    Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Desc.BufferCount = 2;

    Desc.OutputWindow = hWnd;
    Desc.Windowed = TRUE;

    Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT Result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        FeatureLevels,
        ARRAYSIZE(FeatureLevels),
        D3D11_SDK_VERSION,
        &Desc,
        &SwapChain,
        &Device,
        nullptr,
        &DeviceContext
    );

    return SUCCEEDED(Result);
}

bool URenderer::CreateFrameBuffer()
{
    HRESULT Result = SwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&FrameBuffer)
    );

    if (FAILED(Result))
    {
        return false;
    }

    Result = Device->CreateRenderTargetView(
        FrameBuffer,
        nullptr,
        &FrameBufferRTV
    );

    return SUCCEEDED(Result);
}