/**
 * @file Renderer.cpp
 * @brief Direct3D 11 렌더러와 프레임 제출을 제공합니다.
 * @ingroup Renderer
 */

#include "Renderer/Renderer.h"
#include "Renderer/FLineVertex.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Engine/FMeshVertex.h"
#include "Engine/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FDrawLine.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/FRenderStats.h"
#include "FObjectConstants.h"
#include "FGpuMeshVertex.h"
#include "Renderer/FMeshResource.h"
#include "FSceneDrawContext.h"
#include "Core/Base/ErrorHandler.h"
#include "RenderTarget.h"
#include "FScene.h"
#include "FSceneRenderer.h"

#include <d3dcompiler.h>
#include <algorithm>
#include <span>

using namespace Microsoft::WRL;

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 메시 정점 및 픽셀 셰이더의 내장 HLSL 소스입니다.
     */
    constexpr char MeshShader[]{R"(
cbuffer ObjectConstants : register(b0) {
    row_major float4x4 World;
    row_major float4x4 WorldInverse;
    float4 Color;
};
cbuffer ViewConstants : register(b1) {
    row_major float4x4 ViewProjection;
};
struct FInput {
    float3 mPosition : POSITION;
    float3 mNormal : NORMAL;
};
struct FOutput {
    float4 mPosition : SV_POSITION;
    float3 mNormal : NORMAL;
};
FOutput VertexMain(FInput Input) {
    FOutput Output = {float4(0, 0, 0, 0), float3(0, 0, 0)};
    Output.mPosition = mul(mul(float4(Input.mPosition, 1), World), ViewProjection);
    Output.mNormal = normalize(mul(Input.mNormal, transpose((float3x3)WorldInverse)));
    return Output;
}
float4 PixelMain(FOutput Input) : SV_TARGET {
    float Diffuse = {saturate(dot(normalize(Input.mNormal), normalize(float3(-0.35, -0.6, 1.0))))};
    return float4(Color.rgb * (0.32 + 0.68 * Diffuse), 1);
}
)"};

    /**
     * @brief 보조 선분 정점 및 픽셀 셰이더의 내장 HLSL 소스입니다.
     */
    constexpr char LineShader[]{R"(
cbuffer ViewConstants : register(b1) {
    row_major float4x4 ViewProjection;
};
struct FInput {
    float3 mPosition : POSITION;
    float3 mColor : COLOR;
};
struct FOutput {
    float4 mPosition : SV_POSITION;
    float3 mColor : COLOR;
};
FOutput VertexMain(FInput Input) {
    FOutput Output = {float4(0, 0, 0, 0), float3(0, 0, 0)};
    Output.mPosition = mul(float4(Input.mPosition, 1), ViewProjection);
    Output.mColor = Input.mColor;
    return Output;
}
float4 PixelMain(FOutput Input) : SV_TARGET {
    return float4(Input.mColor, 1);
}
)"};

}

/**
 * @brief FRenderer의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FRenderer::FRenderer() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FRenderer::~FRenderer() {
    Shutdown();
}

/**
 * @brief Direct3D 11 장치, 스왑 체인과 렌더 파이프라인을 준비합니다.
 */
bool FRenderer::Initialize(void* WindowHandle, Uint32 Width, Uint32 Height) {
    if (mDevice != nullptr || WindowHandle == nullptr || Width == 0 || Height == 0) {
        ErrorHandler::Fatal("Renderer", "Invalid renderer initialization state");
    }

    DXGI_SWAP_CHAIN_DESC Description{};
    Description.BufferDesc.Width = Width;
    Description.BufferDesc.Height = Height;
    Description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;

    Description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Description.BufferCount = 2;

    Description.OutputWindow = static_cast<HWND>(WindowHandle);
    Description.Windowed = true;
    Description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT Flags{D3D11_CREATE_DEVICE_BGRA_SUPPORT};
#ifdef _DEBUG
    Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL Level{D3D_FEATURE_LEVEL_11_0};
    HRESULT Result{E_FAIL};

    for (Int32 Attempt{}; Attempt < 4 && FAILED(Result); ++Attempt) {
        const D3D_DRIVER_TYPE Driver{Attempt >= 2 ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE};
        const UINT AttemptFlags{(Attempt & 1) != 0 ? Flags & ~D3D11_CREATE_DEVICE_DEBUG : Flags};

        Result = D3D11CreateDeviceAndSwapChain(nullptr, Driver, nullptr, AttemptFlags, &Level, 1, D3D11_SDK_VERSION, &Description, mSwapChain.GetAddressOf(), mDevice.GetAddressOf(), nullptr, mContext.GetAddressOf());
    }

    if (FAILED(Result)) {
        ErrorHandler::ReportHRESULT(Result, "Renderer", "D3D11 device creation failed", ErrorHandler::EErrorLevel::Critical);
    }

    ComPtr<IDXGIFactory> Factory{};

    if (SUCCEEDED(mSwapChain->GetParent(IID_PPV_ARGS(Factory.GetAddressOf())))) {
        Factory->MakeWindowAssociation(Description.OutputWindow, DXGI_MWA_NO_ALT_ENTER);
    }

    if (!CreateBackBuffer(Width, Height) || !CreatePipeline()) {
        ErrorHandler::Fatal("Renderer", "Render pipeline creation failed");
    }

    return true;
}

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void FRenderer::Shutdown() {
    if (mContext != nullptr) {
        mContext->ClearState();
        mContext->Flush();
    }

    mMeshes = {};
    mLineVertices = TArray<FLineVertex>{};

    mViewBuffer.Reset();
    mLineBuffer.Reset();

    mMeshVertexShader.Reset();
    mMeshPixelShader.Reset();
    mMeshLayout.Reset();

    mLineVertexShader.Reset();
    mLinePixelShader.Reset();
    mLineLayout.Reset();

    mRasterizer.Reset();
    mDepthEnabled.Reset();
    mDepthReadOnly.Reset();
    mDepthDisabled.Reset();

    mBackBuffer.Reset();
    mSwapChain.Reset();
    mContext.Reset();
    mDevice.Reset();

    mLineCapacity = 0;
    mWidth = 0;
    mHeight = 0;
    mStats = {};
}

/**
 * @brief 렌더러 장치를 참조하는 새로운 렌더 장면을 생성합니다.
 */
std::unique_ptr<FSceneInterface> FRenderer::AllocateScene() {
    ErrorHandler::Report(mDevice == nullptr, "Renderer", "Scene allocation requires an initialized device", ErrorHandler::EErrorLevel::Critical);

    return std::make_unique<FScene>(mDevice.Get(), mContext.Get());
}

/**
 * @brief CPU 메시를 종류별 GPU 정점·인덱스 버퍼에 업로드합니다.
 */
bool FRenderer::UploadMesh(EPrimitiveType Type, const FMeshData& Mesh) {
    if (Type >= EPrimitiveType::Count || Mesh.mVertices.empty() || Mesh.mIndices.empty() || mDevice == nullptr) {
        ErrorHandler::Fatal("Renderer", "Invalid mesh upload state");
    }

    TArray<FGpuMeshVertex> Vertices{};
    Vertices.reserve(Mesh.mVertices.size());

    for (const FMeshVertex& Vertex : Mesh.mVertices) {
        Vertices.emplace_back(Vertex);
    }

    FMeshResource Resource{};

    D3D11_BUFFER_DESC Description{};
    Description.ByteWidth = static_cast<UINT>(Vertices.size() * sizeof(FGpuMeshVertex));
    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA Data{};
    Data.pSysMem = Vertices.data();

    if (FAILED(mDevice->CreateBuffer(&Description, &Data, Resource.mVertices.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Mesh vertex buffer allocation failed");
    }

    Resource.mBytes = Description.ByteWidth;

    Description.ByteWidth = static_cast<UINT>(Mesh.mIndices.size() * sizeof(Uint32));
    Description.BindFlags = D3D11_BIND_INDEX_BUFFER;
    Data.pSysMem = Mesh.mIndices.data();

    if (FAILED(mDevice->CreateBuffer(&Description, &Data, Resource.mIndices.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Mesh index buffer allocation failed");
    }

    Resource.mBytes += Description.ByteWidth;
    Resource.mIndexCount = static_cast<Uint32>(Mesh.mIndices.size());

    mMeshes[static_cast<std::size_t>(Type)] = std::move(Resource);

    return true;
}

/**
 * @brief 지정한 픽셀 크기의 오프스크린 렌더 타깃을 생성합니다.
 */
std::unique_ptr<FRenderTarget> FRenderer::CreateRenderTarget(Uint32 Width, Uint32 Height) {
    auto Target{std::make_unique<FDXRenderTarget>()};
    return mDevice != nullptr && Target->Resize(mDevice.Get(), Width, Height) ? std::move(Target) : nullptr;
}

/**
 * @brief 기존 오프스크린 렌더 타깃을 새 픽셀 크기로 변경합니다.
 */
bool FRenderer::ResizeRenderTarget(FRenderTarget* Target, Uint32 Width, Uint32 Height) {
    auto* Native{dynamic_cast<FDXRenderTarget*>(Target)};

    if (Native == nullptr || mDevice == nullptr) {
        ErrorHandler::Fatal("Renderer", "Invalid render target resize state");
    }

    mContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* Empty{};
    mContext->PSSetShaderResources(0, 1, &Empty);

    return Native->Resize(mDevice.Get(), Width, Height);
}

/**
 * @brief 백 버퍼 크기를 동기화하고 프레임 렌더링 통계를 초기화합니다.
 */
void FRenderer::BeginFrame(Uint32 Width, Uint32 Height) {
    mStats = {};

    if (Width > 0 && Height > 0 && (mWidth != Width || mHeight != Height)) {
        CreateBackBuffer(Width, Height);
    }
}

/**
 * @brief 지정한 장면 뷰를 타깃에 렌더링하고 에디터 보조 선분을 그립니다.
 */
void FRenderer::BeginRenderingViewFamily(const FSceneViewFamily& Family) {
    const auto* Scene{dynamic_cast<const FScene*>(Family.mScene)};
    auto* Target{dynamic_cast<FDXRenderTarget*>(Family.mTarget)};

    if (Scene == nullptr || Target == nullptr || mContext == nullptr) {
        ErrorHandler::Fatal("Renderer", "Invalid scene view family");
    }

    ID3D11ShaderResourceView* Empty{};
    mContext->PSSetShaderResources(0, 1, &Empty);

    ID3D11RenderTargetView* RenderTarget{Target->GetRenderTargetView()};
    mContext->OMSetRenderTargets(1, &RenderTarget, Target->GetDepthView());

    constexpr float ClearColor[]{0.065f, 0.078f, 0.10f, 1.0f};
    mContext->ClearRenderTargetView(RenderTarget, ClearColor);
    mContext->ClearDepthStencilView(Target->GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    const D3D11_VIEWPORT Viewport{0.0f, 0.0f, static_cast<float>(Target->GetWidth()), static_cast<float>(Target->GetHeight()), 0.0f, 1.0f};
    mContext->RSSetViewports(1, &Viewport);
    mContext->RSSetState(mRasterizer.Get());
    mContext->OMSetDepthStencilState(mDepthEnabled.Get(), 0);
    mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);

    mContext->GSSetShader(nullptr, nullptr, 0);
    mContext->HSSetShader(nullptr, nullptr, 0);
    mContext->DSSetShader(nullptr, nullptr, 0);

    const FSceneDrawContext Context{mContext.Get(), &mMeshes, mMeshVertexShader.Get(), mMeshPixelShader.Get(), mMeshLayout.Get(), mViewBuffer.Get()};
    FSceneRenderer SceneRenderer{Family, Scene, Context, mStats};
    SceneRenderer.Render();

    if (Family.mEditorElements != nullptr) {
        DrawLines(Family.mEditorElements, true);
        DrawLines(Family.mEditorElements, false);
    }

    mStats.mGpuBytes = Scene->GetPrimitiveCount() * sizeof(FObjectConstants) + sizeof(FMatrix) + mLineCapacity * sizeof(FLineVertex) + static_cast<std::size_t>(Target->GetWidth()) * Target->GetHeight() * 8;

    for (const FMeshResource& Mesh : mMeshes) {
        mStats.mGpuBytes += Mesh.mBytes;
    }

    mContext->OMSetRenderTargets(0, nullptr, nullptr);
}

/**
 * @brief UI 렌더링에 사용할 백 버퍼와 뷰포트를 설정합니다.
 */
void FRenderer::BeginUiRender() {
    if (mBackBuffer != nullptr) {
        ID3D11RenderTargetView* Target{mBackBuffer.Get()};
        mContext->OMSetRenderTargets(1, &Target, nullptr);

        constexpr float Clear[]{0.045f, 0.05f, 0.065f, 1.0f};
        mContext->ClearRenderTargetView(Target, Clear);
    }
}

/**
 * @brief 스왑 체인의 백 버퍼를 화면에 표시합니다.
 */
void FRenderer::Present() {
    if (mSwapChain != nullptr && FAILED(mSwapChain->Present(1, 0))) {
        ErrorHandler::Fatal("Renderer", "Presentation failed");
    }
}

/**
 * @brief Direct3D 11 장치의 비소유 주소를 반환합니다.
 */
void* FRenderer::GetNativeDevice() const {
    return mDevice.Get();
}

/**
 * @brief Direct3D 11 즉시 컨텍스트의 비소유 주소를 반환합니다.
 */
void* FRenderer::GetNativeContext() const {
    return mContext.Get();
}

/**
 * @brief 프레임 렌더링 통계를 반환합니다.
 */
FRenderStats FRenderer::GetStats() const {
    return mStats;
}

/**
 * @brief 내장 HLSL 소스를 지정한 진입점과 프로파일로 컴파일합니다.
 */
bool FRenderer::Compile(const char* Source, std::size_t Size, const char* Entry, const char* Target, ComPtr<ID3DBlob>& Code) {
    ComPtr<ID3DBlob> Errors{};
    const HRESULT Result{D3DCompile(Source, Size, "Macaw", nullptr, nullptr, Entry, Target, D3DCOMPILE_ENABLE_STRICTNESS, 0, Code.GetAddressOf(), Errors.GetAddressOf())};

    if (FAILED(Result)) {
        ErrorHandler::Fatal("Renderer", Errors != nullptr ? FString{static_cast<const char*>(Errors->GetBufferPointer()), Errors->GetBufferSize()} : "Shader compilation failed");
    }

    return true;
}

/**
 * @brief 스왑 체인을 조정하고 백 버퍼 렌더 타깃 뷰를 생성합니다.
 */
bool FRenderer::CreateBackBuffer(Uint32 Width, Uint32 Height) {
    mContext->OMSetRenderTargets(0, nullptr, nullptr);
    mBackBuffer.Reset();

    if (FAILED(mSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0))) {
        ErrorHandler::Fatal("Renderer", "Swap chain resize failed");
    }

    ComPtr<ID3D11Texture2D> Texture{};

    if (FAILED(mSwapChain->GetBuffer(0, IID_PPV_ARGS(Texture.GetAddressOf()))) || FAILED(mDevice->CreateRenderTargetView(Texture.Get(), nullptr, mBackBuffer.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Back buffer allocation failed");
    }

    mWidth = Width;
    mHeight = Height;

    return true;
}

/**
 * @brief 메시·선분 셰이더, 입력 레이아웃과 고정 파이프라인 상태를 생성합니다.
 */
bool FRenderer::CreatePipeline() {
    ComPtr<ID3DBlob> VertexCode{};
    ComPtr<ID3DBlob> PixelCode{};

    if (!Compile(MeshShader, sizeof(MeshShader), "VertexMain", "vs_5_0", VertexCode) || !Compile(MeshShader, sizeof(MeshShader), "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 2> MeshElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FGpuMeshVertex, mPosition), D3D11_INPUT_PER_VERTEX_DATA, 0}, {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FGpuMeshVertex, mNormal), D3D11_INPUT_PER_VERTEX_DATA, 0}}};

    if (FAILED(mDevice->CreateVertexShader(VertexCode->GetBufferPointer(), VertexCode->GetBufferSize(), nullptr, mMeshVertexShader.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetBufferPointer(), PixelCode->GetBufferSize(), nullptr, mMeshPixelShader.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(MeshElements.data(), static_cast<UINT>(MeshElements.size()), VertexCode->GetBufferPointer(), VertexCode->GetBufferSize(), mMeshLayout.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Mesh pipeline allocation failed");
    }

    VertexCode.Reset();
    PixelCode.Reset();

    if (!Compile(LineShader, sizeof(LineShader), "VertexMain", "vs_5_0", VertexCode) || !Compile(LineShader, sizeof(LineShader), "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 2> LineElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FLineVertex, mPosition), D3D11_INPUT_PER_VERTEX_DATA, 0}, {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FLineVertex, mColor), D3D11_INPUT_PER_VERTEX_DATA, 0}}};

    if (FAILED(mDevice->CreateVertexShader(VertexCode->GetBufferPointer(), VertexCode->GetBufferSize(), nullptr, mLineVertexShader.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetBufferPointer(), PixelCode->GetBufferSize(), nullptr, mLinePixelShader.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(LineElements.data(), static_cast<UINT>(LineElements.size()), VertexCode->GetBufferPointer(), VertexCode->GetBufferSize(), mLineLayout.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Line pipeline allocation failed");
    }

    D3D11_BUFFER_DESC Buffer{};
    Buffer.ByteWidth = sizeof(FMatrix);
    Buffer.Usage = D3D11_USAGE_DEFAULT;
    Buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    if (FAILED(mDevice->CreateBuffer(&Buffer, nullptr, mViewBuffer.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "View buffer allocation failed");
    }

    D3D11_RASTERIZER_DESC Rasterizer{};
    Rasterizer.FillMode = D3D11_FILL_SOLID;
    Rasterizer.CullMode = D3D11_CULL_NONE;
    Rasterizer.DepthClipEnable = true;
    Rasterizer.AntialiasedLineEnable = true;

    if (FAILED(mDevice->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Rasterizer allocation failed");
    }

    D3D11_DEPTH_STENCIL_DESC Depth{};
    Depth.DepthEnable = true;
    Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    Depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    if (FAILED(mDevice->CreateDepthStencilState(&Depth, mDepthEnabled.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Depth state allocation failed");
    }

    Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

    if (FAILED(mDevice->CreateDepthStencilState(&Depth, mDepthReadOnly.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Depth state allocation failed");
    }

    Depth.DepthEnable = false;
    ErrorHandler::ReportHRESULT(mDevice->CreateDepthStencilState(&Depth, mDepthDisabled.GetAddressOf()), "Renderer", "Depth state allocation failed", ErrorHandler::EErrorLevel::Critical);

    return true;
}

/**
 * @brief 깊이 검사 정책에 맞는 보조 선분을 동적 정점 버퍼로 그립니다.
 */
void FRenderer::DrawLines(const FPrimitiveDrawInterface* Elements, bool DepthTest) {
    mLineVertices.clear();

    for (const FDrawLine& Line : Elements->GetLines()) {
        if (Line.mDepthTest == DepthTest) {
            mLineVertices.push_back({Line.mStart, Line.mColor});
            mLineVertices.push_back({Line.mEnd, Line.mColor});
        }
    }

    if (mLineVertices.empty()) {
        return;
    }

    const Uint32 Count{static_cast<Uint32>(mLineVertices.size())};

    if (Count > mLineCapacity) {
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = (Count + 256) * sizeof(FLineVertex);
        Description.Usage = D3D11_USAGE_DYNAMIC;
        Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        ComPtr<ID3D11Buffer> Buffer{};

        if (FAILED(mDevice->CreateBuffer(&Description, nullptr, Buffer.GetAddressOf()))) {
            ErrorHandler::Fatal("Renderer", "Line buffer allocation failed");
            return;
        }

        mLineBuffer = std::move(Buffer);
        mLineCapacity = Count + 256;
    }

    D3D11_MAPPED_SUBRESOURCE Mapped{};

    if (FAILED(mContext->Map(mLineBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped))) {
        ErrorHandler::Fatal("Renderer", "Line buffer mapping failed");
    }

    std::ranges::copy(std::as_bytes(std::span{mLineVertices}), static_cast<std::byte*>(Mapped.pData));
    mContext->Unmap(mLineBuffer.Get(), 0);

    const UINT Stride{sizeof(FLineVertex)};
    const UINT Offset{};
    ID3D11Buffer* Buffer{mLineBuffer.Get()};

    mContext->IASetInputLayout(mLineLayout.Get());
    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    mContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);

    mContext->VSSetShader(mLineVertexShader.Get(), nullptr, 0);
    mContext->PSSetShader(mLinePixelShader.Get(), nullptr, 0);
    mContext->OMSetDepthStencilState(DepthTest ? mDepthReadOnly.Get() : mDepthDisabled.Get(), 0);

    mContext->Draw(Count, 0);

    ++mStats.mDrawCalls;
}
