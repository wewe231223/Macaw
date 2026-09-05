#pragma once

#include <Windows.h>
#include <d3d11.h>

class URenderer
{
public:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    IDXGISwapChain* SwapChain = nullptr;

    ID3D11Texture2D* FrameBuffer = nullptr;
    ID3D11RenderTargetView* FrameBufferRTV = nullptr;

public:
    bool Create(HWND hWnd);

    void Prepare();
    void Present();
    void Release();

private:
    bool CreateDeviceAndSwapChain(HWND hWnd);
    bool CreateFrameBuffer();
};