#include "PCH.h"

#include "Renderer.h"
#include "../ErrorHandler.h"

#include "Pipeline/Pipeline.h"

FRenderer::~FRenderer() {

}

void FRenderer::Create(HWND WindowHandle, UINT width, UINT height) {
	Width = width;
	Height = height;

	FRenderer::CreateDeviceAndSwapChain(WindowHandle);
	FRenderer::CreateRTV();
	FRenderer::CreateDSV();

	Pipeline t{};
	ErrorHandler::Report(not t.Initialize(Device.Get(), "./Pipeline/Base.json"), "[ FRenderer ]", "Failed to initialize pipeline.", ErrorHandler::EErrorLevel::Critical);
}

void FRenderer::BeginFrame() {
	DeviceContext->ClearRenderTargetView(RenderTargetView.Get(), ClearColor);
	DeviceContext->ClearDepthStencilView(DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	DeviceContext->OMSetRenderTargets(1, RenderTargetView.GetAddressOf(), DepthStencilView.Get());
	
	DeviceContext->RSSetViewports(1, &Viewport);
}

void FRenderer::EndFrame() {
	SwapChain->Present(1, 0);
}

void FRenderer::CreateDeviceAndSwapChain(HWND WindowHandle) {
	// 지원하는 Direct3D 기능 레벨을 정의
	D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

	// 스왑 체인 설정 구조체 초기화
	DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
	swapchaindesc.BufferDesc.Width = Width; // 창 크기에 맞게 자동으로 설정
	swapchaindesc.BufferDesc.Height = Height; // 창 크기에 맞게 자동으로 설정
	swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
	swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
	swapchaindesc.BufferCount = 2; // 더블 버퍼링
	swapchaindesc.OutputWindow = WindowHandle; // 렌더링할 창 핸들
	swapchaindesc.Windowed = TRUE; // 창 모드
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

	// Direct3D 장치와 스왑 체인을 생성
	ErrorHandler::ReportHRESULT(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
		featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
		&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext), "[ FRenderer ]", "Failed to create Direct3D device and swap chain.", ErrorHandler::EErrorLevel::Critical);

	// 생성된 스왑 체인의 정보 가져오기
	SwapChain->GetDesc(&swapchaindesc);

	// 뷰포트 정보 설정
	Viewport = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
}

void FRenderer::CreateRTV() {
	// 스왑 체인으로부터 백 버퍼 텍스처 가져오기
	SwapChain->GetBuffer(0, IID_PPV_ARGS(BackBuffer.GetAddressOf()));

	// 렌더 타겟 뷰 생성
	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; // 색상 포맷
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처

	ErrorHandler::ReportHRESULT(Device->CreateRenderTargetView(BackBuffer.Get(), &framebufferRTVdesc, &RenderTargetView), "[ FRenderer ]", "Failed to create render target view.", ErrorHandler::EErrorLevel::Critical);
}

void FRenderer::CreateDSV() {
	D3D11_TEXTURE2D_DESC TextureDesc{};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	ErrorHandler::ReportHRESULT(Device->CreateTexture2D(&TextureDesc, nullptr, DepthStencilBuffer.GetAddressOf()), "[ FRenderer ]", "Failed to create depth stencil buffer.", ErrorHandler::EErrorLevel::Critical);

	D3D11_DEPTH_STENCIL_VIEW_DESC ViewDesc{};
	ViewDesc.Format = TextureDesc.Format;
	ViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	ViewDesc.Texture2D.MipSlice = 0;

	ErrorHandler::ReportHRESULT(Device->CreateDepthStencilView(DepthStencilBuffer.Get(), &ViewDesc, DepthStencilView.GetAddressOf()), "[ FRenderer ]", "Failed to create depth stencil view.", ErrorHandler::EErrorLevel::Critical);
}
