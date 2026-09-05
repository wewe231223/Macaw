#pragma once 
#include <d3d11.h>
#include <wrl/client.h>

class FRenderer {
public:
	FRenderer() = default;
	~FRenderer();
	
	FRenderer(const FRenderer&) = delete;
	FRenderer& operator=(const FRenderer&) = delete;

	FRenderer(FRenderer&&) = delete;
	FRenderer& operator=(FRenderer&&) = delete;

public:
	void Create(HWND WindowHandle, UINT width, UINT height);

	void BeginFrame();
	void EndFrame();
private:
	void CreateDeviceAndSwapChain(HWND WindowHandle);
	
	void CreateRTV();
	void CreateDSV();

private:
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;

	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
	
	Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBuffer;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
	
	Microsoft::WRL::ComPtr<ID3D11Texture2D> DepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView;

	const float ClearColor[4] = { 0.2f, 0.2f, 0.7f, 1.0f };
	D3D11_VIEWPORT Viewport{};

	UINT Width{ 0 };
	UINT Height{ 0 };
};