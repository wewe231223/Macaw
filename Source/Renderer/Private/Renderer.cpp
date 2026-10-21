#include "Renderer/Renderer.h"
#include "FLineVertex.h"
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
#include "FMeshResource.h"
#include "FSceneDrawContext.h"
#include "Core/Base/ErrorHandler.h"
#include "RenderTarget.h"
#include "FScene.h"
#include "FSceneRenderer.h"

#include <d3dcompiler.h>
#include <algorithm>
#include <span>

using namespace Microsoft::WRL;

namespace {
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

struct FRenderer::FImplementation {
private:
    friend class FRenderer;

public:
    bool CreateBackBuffer(Uint32 Width, Uint32 Height);
    bool CreatePipeline();
    bool Compile(const char* Source, std::size_t Size, const char* Entry, const char* Target, ComPtr<ID3DBlob>& Code);
    void DrawLines(const FPrimitiveDrawInterface& Elements, bool DepthTest);

private:
    ComPtr<ID3D11Device> mDevice{};
    ComPtr<ID3D11DeviceContext> mContext{};

    ComPtr<IDXGISwapChain> mSwapChain{};
    ComPtr<ID3D11RenderTargetView> mBackBuffer{};

    ComPtr<ID3D11VertexShader> mMeshVertexShader{};
    ComPtr<ID3D11PixelShader> mMeshPixelShader{};
    ComPtr<ID3D11InputLayout> mMeshLayout{};

    ComPtr<ID3D11VertexShader> mLineVertexShader{};
    ComPtr<ID3D11PixelShader> mLinePixelShader{};
    ComPtr<ID3D11InputLayout> mLineLayout{};

    ComPtr<ID3D11Buffer> mViewBuffer{};
    ComPtr<ID3D11Buffer> mLineBuffer{};

    ComPtr<ID3D11RasterizerState> mRasterizer{};
    ComPtr<ID3D11DepthStencilState> mDepthEnabled{};
    ComPtr<ID3D11DepthStencilState> mDepthReadOnly{};
    ComPtr<ID3D11DepthStencilState> mDepthDisabled{};

    std::array<FMeshResource, 3> mMeshes{};

    TArray<FLineVertex> mLineVertices{};
    Uint32 mLineCapacity{};

    Uint32 mWidth{};
    Uint32 mHeight{};

    FRenderStats mStats{};
};

bool FRenderer::FImplementation::Compile(const char* Source, std::size_t Size, const char* Entry, const char* Target, ComPtr<ID3DBlob>& Code) {
    ComPtr<ID3DBlob> Errors{};
    const HRESULT Result{D3DCompile(Source, Size, "MacawWeek1", nullptr, nullptr, Entry, Target, D3DCOMPILE_ENABLE_STRICTNESS, 0, Code.GetAddressOf(), Errors.GetAddressOf())};

    if (FAILED(Result)) {
        ErrorHandler::Fatal("Renderer", Errors != nullptr ? FString{static_cast<const char*>(Errors->GetBufferPointer()), Errors->GetBufferSize()} : "Shader compilation failed");
    }

    return true;
}

bool FRenderer::FImplementation::CreateBackBuffer(Uint32 Width, Uint32 Height) {
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

bool FRenderer::FImplementation::CreatePipeline() {
    ComPtr<ID3DBlob> VertexCode{};
    ComPtr<ID3DBlob> PixelCode{};

    if (!Compile(MeshShader, sizeof(MeshShader), "VertexMain", "vs_5_0", VertexCode) || !Compile(MeshShader, sizeof(MeshShader), "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 2> MeshElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}}};

    if (FAILED(mDevice->CreateVertexShader(VertexCode->GetBufferPointer(), VertexCode->GetBufferSize(), nullptr, mMeshVertexShader.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetBufferPointer(), PixelCode->GetBufferSize(), nullptr, mMeshPixelShader.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(MeshElements.data(), static_cast<UINT>(MeshElements.size()), VertexCode->GetBufferPointer(), VertexCode->GetBufferSize(), mMeshLayout.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Mesh pipeline allocation failed");
    }

    VertexCode.Reset();
    PixelCode.Reset();

    if (!Compile(LineShader, sizeof(LineShader), "VertexMain", "vs_5_0", VertexCode) || !Compile(LineShader, sizeof(LineShader), "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 2> LineElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}}};

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

void FRenderer::FImplementation::DrawLines(const FPrimitiveDrawInterface& Elements, bool DepthTest) {
    mLineVertices.clear();

    for (const FDrawLine& Line : Elements.GetLines()) {
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

FRenderer::FRenderer()
	: mImplementation{std::make_unique<FImplementation>()} {
}

FRenderer::~FRenderer() {
    Shutdown();
}

bool FRenderer::Initialize(void* WindowHandle, Uint32 Width, Uint32 Height) {
    if (mImplementation->mDevice != nullptr || WindowHandle == nullptr || Width == 0 || Height == 0) {
        ErrorHandler::Fatal("Renderer", "Invalid renderer initialization state");
    }

    auto& State{*mImplementation};

    DXGI_SWAP_CHAIN_DESC Description{};
    Description.BufferDesc.Width = Width;
    Description.BufferDesc.Height = Height;
    Description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;

    Description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Description.BufferCount = 2;

    Description.OutputWindow = static_cast<HWND>(WindowHandle);
    Description.Windowed = true;
    Description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT Flags{D3D11_CREATE_DEVICE_BGRA_SUPPORT};
#ifdef _DEBUG
    Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL Level{D3D_FEATURE_LEVEL_11_0};
    HRESULT Result{E_FAIL};

    for (Int32 Attempt{}; Attempt < 4 && FAILED(Result); ++Attempt) {
        const D3D_DRIVER_TYPE Driver{Attempt >= 2 ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE};
        const UINT AttemptFlags{(Attempt & 1) != 0 ? Flags & ~D3D11_CREATE_DEVICE_DEBUG : Flags};

        Result = D3D11CreateDeviceAndSwapChain(nullptr, Driver, nullptr, AttemptFlags, &Level, 1, D3D11_SDK_VERSION, &Description, State.mSwapChain.GetAddressOf(), State.mDevice.GetAddressOf(), nullptr, State.mContext.GetAddressOf());
    }

    if (FAILED(Result)) {
        ErrorHandler::ReportHRESULT(Result, "Renderer", "D3D11 device creation failed", ErrorHandler::EErrorLevel::Critical);
    }

    ComPtr<IDXGIFactory> Factory{};

    if (SUCCEEDED(State.mSwapChain->GetParent(IID_PPV_ARGS(Factory.GetAddressOf())))) {
        Factory->MakeWindowAssociation(Description.OutputWindow, DXGI_MWA_NO_ALT_ENTER);
    }

    if (!State.CreateBackBuffer(Width, Height) || !State.CreatePipeline()) {
        ErrorHandler::Fatal("Renderer", "Render pipeline creation failed");
    }

    return true;
}

void FRenderer::Shutdown() {
    if (mImplementation != nullptr && mImplementation->mContext != nullptr) {
        mImplementation->mContext->ClearState();
        mImplementation->mContext->Flush();
    }

    *mImplementation = FImplementation{};
}

std::unique_ptr<FSceneInterface> FRenderer::AllocateScene() {
    ErrorHandler::Report(mImplementation->mDevice == nullptr, "Renderer", "Scene allocation requires an initialized device", ErrorHandler::EErrorLevel::Critical);

    return std::make_unique<FScene>(*mImplementation->mDevice.Get(), *mImplementation->mContext.Get());
}

bool FRenderer::UploadMesh(EPrimitiveType Type, const FMeshData& Mesh) {
    if (Type >= EPrimitiveType::Count || Mesh.mVertices.empty() || Mesh.mIndices.empty() || mImplementation->mDevice == nullptr) {
        ErrorHandler::Fatal("Renderer", "Invalid mesh upload state");
    }

    FMeshResource Resource{};

    D3D11_BUFFER_DESC Description{};
    Description.ByteWidth = static_cast<UINT>(Mesh.mVertices.size() * sizeof(FMeshVertex));
    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA Data{};
    Data.pSysMem = Mesh.mVertices.data();

    if (FAILED(mImplementation->mDevice->CreateBuffer(&Description, &Data, Resource.mVertices.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Mesh vertex buffer allocation failed");
    }

    Resource.mBytes = Description.ByteWidth;

    Description.ByteWidth = static_cast<UINT>(Mesh.mIndices.size() * sizeof(Uint32));
    Description.BindFlags = D3D11_BIND_INDEX_BUFFER;
    Data.pSysMem = Mesh.mIndices.data();

    if (FAILED(mImplementation->mDevice->CreateBuffer(&Description, &Data, Resource.mIndices.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Mesh index buffer allocation failed");
    }

    Resource.mBytes += Description.ByteWidth;
    Resource.mIndexCount = static_cast<Uint32>(Mesh.mIndices.size());

    mImplementation->mMeshes[static_cast<std::size_t>(Type)] = std::move(Resource);

    return true;
}

std::unique_ptr<FRenderTarget> FRenderer::CreateRenderTarget(Uint32 Width, Uint32 Height) {
    auto Target{std::make_unique<FDXRenderTarget>()};
    return mImplementation->mDevice != nullptr && Target->Resize(*mImplementation->mDevice.Get(), Width, Height) ? std::move(Target) : nullptr;
}

bool FRenderer::ResizeRenderTarget(FRenderTarget& Target, Uint32 Width, Uint32 Height) {
    auto* Native{dynamic_cast<FDXRenderTarget*>(&Target)};

    if (Native == nullptr || mImplementation->mDevice == nullptr) {
        ErrorHandler::Fatal("Renderer", "Invalid render target resize state");
    }

    mImplementation->mContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* Empty{};
    mImplementation->mContext->PSSetShaderResources(0, 1, &Empty);

    return Native->Resize(*mImplementation->mDevice.Get(), Width, Height);
}

void FRenderer::BeginFrame(Uint32 Width, Uint32 Height) {
    auto& State{*mImplementation};
    State.mStats = {};

    if (Width > 0 && Height > 0 && (State.mWidth != Width || State.mHeight != Height)) {
        State.CreateBackBuffer(Width, Height);
    }
}

void FRenderer::BeginRenderingViewFamily(const FSceneViewFamily& Family) {
    auto& State{*mImplementation};
    const auto* Scene{dynamic_cast<const FScene*>(Family.mScene)};
    auto* Target{dynamic_cast<FDXRenderTarget*>(Family.mTarget)};

    if (Scene == nullptr || Target == nullptr || State.mContext == nullptr) {
        ErrorHandler::Fatal("Renderer", "Invalid scene view family");
    }

    ID3D11ShaderResourceView* Empty{};
    State.mContext->PSSetShaderResources(0, 1, &Empty);

    ID3D11RenderTargetView* RenderTarget{Target->GetRenderTargetView()};
    State.mContext->OMSetRenderTargets(1, &RenderTarget, Target->GetDepthView());

    constexpr float ClearColor[]{0.065f, 0.078f, 0.10f, 1.0f};
    State.mContext->ClearRenderTargetView(RenderTarget, ClearColor);
    State.mContext->ClearDepthStencilView(Target->GetDepthView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    const D3D11_VIEWPORT Viewport{0.0f, 0.0f, static_cast<float>(Target->GetWidth()), static_cast<float>(Target->GetHeight()), 0.0f, 1.0f};
    State.mContext->RSSetViewports(1, &Viewport);
    State.mContext->RSSetState(State.mRasterizer.Get());
    State.mContext->OMSetDepthStencilState(State.mDepthEnabled.Get(), 0);
    State.mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);

    State.mContext->GSSetShader(nullptr, nullptr, 0);
    State.mContext->HSSetShader(nullptr, nullptr, 0);
    State.mContext->DSSetShader(nullptr, nullptr, 0);

    const FSceneDrawContext Context{State.mContext.Get(), &State.mMeshes, State.mMeshVertexShader.Get(), State.mMeshPixelShader.Get(), State.mMeshLayout.Get(), State.mViewBuffer.Get()};
    FSceneRenderer SceneRenderer{Family, *Scene, Context, State.mStats};
    SceneRenderer.Render();

    if (Family.mEditorElements != nullptr) {
        State.DrawLines(*Family.mEditorElements, true);
        State.DrawLines(*Family.mEditorElements, false);
    }

    State.mStats.mGpuBytes = Scene->GetPrimitiveCount() * sizeof(FObjectConstants) + sizeof(FMatrix) + State.mLineCapacity * sizeof(FLineVertex) + static_cast<std::size_t>(Target->GetWidth()) * Target->GetHeight() * 8;

    for (const FMeshResource& Mesh : State.mMeshes) {
        State.mStats.mGpuBytes += Mesh.mBytes;
    }

    State.mContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FRenderer::BeginUiRender() {
    auto& State{*mImplementation};

    if (State.mBackBuffer != nullptr) {
        ID3D11RenderTargetView* Target{State.mBackBuffer.Get()};
        State.mContext->OMSetRenderTargets(1, &Target, nullptr);

        constexpr float Clear[]{0.045f, 0.05f, 0.065f, 1.0f};
        State.mContext->ClearRenderTargetView(Target, Clear);
    }
}

void FRenderer::Present() {
    if (mImplementation->mSwapChain != nullptr && FAILED(mImplementation->mSwapChain->Present(1, 0))) {
        ErrorHandler::Fatal("Renderer", "Presentation failed");
    }
}

void* FRenderer::GetNativeDevice() const {
    return mImplementation->mDevice.Get();
}

void* FRenderer::GetNativeContext() const {
    return mImplementation->mContext.Get();
}

FRenderStats FRenderer::GetStats() const {
    return mImplementation->mStats;
}
