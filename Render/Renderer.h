#pragma once 
#include <d3d11.h>
#include <wrl/client.h>

#include "../Core/Base/FRenderProbe.h"
#include "../Core/Asset/FAssetRegistry.h"

#include "Pipeline/UPipeline.h"
#include "../Core/Asset/UMaterial.h"
#include "../Core/Asset/UMesh.h"

class FRenderer {
	struct FRenderBatch {
		TFixedArray<ID3D11Buffer*, static_cast<size_t>(EVertexAttribute::MAX)> VertexBuffers{};

		UPipeline* Pipeline{ nullptr };

		uint32 InstanceCount{ 0 };
		TArray<FMatrix> World{}; 
	};
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

	ID3D11Device* GetDevice() const { return Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext.Get(); }

	void BindAssetRegistry(FAssetRegistry* InAssetRegistry) { AssetRegistry = InAssetRegistry; }
	void AcceptRenderProbe(const FRenderProbe& Probe);

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

	FAssetRegistry* AssetRegistry{ nullptr };

	const float ClearColor[4] = { 0.2f, 0.2f, 0.7f, 1.0f };
	D3D11_VIEWPORT Viewport{};

	UINT Width{ 0 };
	UINT Height{ 0 };
};