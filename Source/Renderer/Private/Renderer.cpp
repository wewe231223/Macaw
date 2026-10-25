/**
 * @file Renderer.cpp
 * @brief Direct3D 11 렌더러와 프레임 제출을 제공합니다.
 * @ingroup Renderer
 */

#include "Renderer/Renderer.h"
#include "Renderer/FLineVertex.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Engine/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FDrawLine.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/FRenderStats.h"
#include "FObjectConstants.h"
#include "Engine/UStaticMesh.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "FSceneDrawContext.h"
#include "Core/Base/ErrorHandler.h"
#include "RenderTarget.h"
#include "FScene.h"
#include "FSceneRenderer.h"
#include "FEditorMeshConstants.h"
#include "Engine/EditorMesh.h"
#include "Core/Math/FVector4.h"

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
    constexpr char MeshShader[]{"Content/Shaders/Mesh.hlsl"};

    /**
     * @brief 보조 선분 정점 및 픽셀 셰이더의 내장 HLSL 소스입니다.
     */
    constexpr char LineShader[]{"Content/Shaders/Line.hlsl"};

    /**
     * @brief 기즈모의 월드 행렬 변환과 조명 없는 RGBA 출력을 수행하는 내장 HLSL 소스입니다.
     */
    constexpr char GizmoShader[]{"Content/Shaders/Gizmo.hlsl"};

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

    if (!CreateBackBuffer(Width, Height) || !CreatePipeline() || !CreateEditorMeshes()) {
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
    mEditorMeshes = {};
    mLineVertices = TArray<FLineVertex>{};

    mOverlayLayout.Reset();
    mEditorMeshBuffer.Reset();
    mOverlayBlend.Reset();
    mViewBuffer.Reset();
    mLineBuffer.Reset();

    mMeshVertexShader.Reset();
    mMeshPixelShader.Reset();
    mMeshLayout.Reset();

    mLineVertexShader.Reset();
    mLinePixelShader.Reset();
    mLineLayout.Reset();

    mGizmoVertexShader.Reset();
    mGizmoPixelShader.Reset();
    mShaderManager.Clear();
    mShadersDirty = false;

    mRasterizer.Reset();
    mDepthEnabled.Reset();
    mDepthReadOnly.Reset();
    mDepthDisabled.Reset();

    mBackBuffer.reset();
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
    if (Type >= EPrimitiveType::Count || mDevice == nullptr) {
        return false;
    }

    auto Resource{NewObject<UStaticMesh>()};

    if (!Resource->SetMeshData(Mesh) || !Resource->CreateResources(mDevice.Get())) {
        return false;
    }

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
        DrawMeshes(Family.mEditorElements, true);
        DrawLines(Family.mEditorElements, true);
        mContext->ClearDepthStencilView(Target->GetDepthView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        DrawMeshes(Family.mEditorElements, false);
        DrawLines(Family.mEditorElements, false);
    }

    mStats.mGpuBytes = Scene->GetPrimitiveCount() * sizeof(FObjectConstants) + sizeof(FMatrix) + sizeof(FEditorMeshConstants) + mLineCapacity * sizeof(FLineVertex) + Target->GetResourceBytes();

    for (const auto& Mesh : mMeshes) {
        mStats.mGpuBytes += Mesh != nullptr ? Mesh->GetResourceBytes() : 0;
    }
    for (const auto& Mesh : mEditorMeshes) {
        mStats.mGpuBytes += Mesh != nullptr ? Mesh->GetResourceBytes() : 0;
    }

    mContext->OMSetRenderTargets(0, nullptr, nullptr);
}

/**
 * @brief UI 렌더링에 사용할 백 버퍼와 뷰포트를 설정합니다.
 */
void FRenderer::BeginUiRender() {
    if (mBackBuffer != nullptr && mBackBuffer->HasResources()) {
        ID3D11RenderTargetView* Target{mBackBuffer->GetRenderTargetView()};
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

bool FRenderer::ReloadChangedShaders() {
    if (mDevice == nullptr) {
        return false;
    }

    TArray<std::shared_ptr<const FShader>> ChangedShaders{};
    FString Diagnostics{};
    const bool Success{mShaderManager.ReloadChanged(ChangedShaders, Diagnostics)};
    mShadersDirty = mShadersDirty || !ChangedShaders.empty();
    if (!Diagnostics.empty()) {
        ErrorHandler::Report("Shader", Diagnostics, Success ? ErrorHandler::EErrorLevel::Warning : ErrorHandler::EErrorLevel::Error);
    }

    return Success && (!mShadersDirty || CreateShaders());
}

/**
 * @brief 내장 HLSL 소스를 지정한 진입점과 프로파일로 컴파일합니다.
 */
bool FRenderer::Compile(const char* Source, const char* Entry, const char* Target, std::shared_ptr<const FShader>& Code) {
    FShaderDescription Description{};
    Description.mSourcePath = Source;
    Description.mEntryPoint = Entry;
    Description.mTargetProfile = Target;
#ifdef _DEBUG
    Description.mCompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    FString Diagnostics{};
    Code = mShaderManager.Load(Description, Diagnostics);
    if (!Diagnostics.empty()) {
        ErrorHandler::Report("Shader", Diagnostics, Code != nullptr ? ErrorHandler::EErrorLevel::Warning : ErrorHandler::EErrorLevel::Error);
    }

    return Code != nullptr;
}

bool FRenderer::CreateShaders() {
    std::shared_ptr<const FShader> VertexCode{};
    std::shared_ptr<const FShader> PixelCode{};
    ComPtr<ID3D11VertexShader> MeshVertex{};
    ComPtr<ID3D11PixelShader> MeshPixel{};
    ComPtr<ID3D11InputLayout> MeshLayout{};
    ComPtr<ID3D11VertexShader> LineVertex{};
    ComPtr<ID3D11PixelShader> LinePixel{};
    ComPtr<ID3D11InputLayout> LineLayout{};
    ComPtr<ID3D11VertexShader> GizmoVertex{};
    ComPtr<ID3D11PixelShader> GizmoPixel{};
    ComPtr<ID3D11InputLayout> OverlayLayout{};

    if (!Compile(MeshShader, "VertexMain", "vs_5_0", VertexCode) || !Compile(MeshShader, "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 3> MeshElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}}};

    if (FAILED(mDevice->CreateVertexShader(VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), nullptr, MeshVertex.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetByteCode().data(), PixelCode->GetByteCode().size(), nullptr, MeshPixel.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(MeshElements.data(), static_cast<UINT>(MeshElements.size()), VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), MeshLayout.GetAddressOf()))) {
        ErrorHandler::Report("Renderer", "Mesh pipeline allocation failed", ErrorHandler::EErrorLevel::Error);
        return false;
    }

    VertexCode.reset();
    PixelCode.reset();

    if (!Compile(LineShader, "VertexMain", "vs_5_0", VertexCode) || !Compile(LineShader, "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 2> LineElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FLineVertex, mPosition), D3D11_INPUT_PER_VERTEX_DATA, 0}, {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FLineVertex, mColor), D3D11_INPUT_PER_VERTEX_DATA, 0}}};

    if (FAILED(mDevice->CreateVertexShader(VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), nullptr, LineVertex.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetByteCode().data(), PixelCode->GetByteCode().size(), nullptr, LinePixel.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(LineElements.data(), static_cast<UINT>(LineElements.size()), VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), LineLayout.GetAddressOf()))) {
        ErrorHandler::Report("Renderer", "Line pipeline allocation failed", ErrorHandler::EErrorLevel::Error);
        return false;
    }

    VertexCode.reset();
    PixelCode.reset();

    if (!Compile(GizmoShader, "VertexMain", "vs_5_0", VertexCode) || !Compile(GizmoShader, "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 1> OverlayElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}}};
    if (FAILED(mDevice->CreateVertexShader(VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), nullptr, GizmoVertex.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetByteCode().data(), PixelCode->GetByteCode().size(), nullptr, GizmoPixel.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(OverlayElements.data(), static_cast<UINT>(OverlayElements.size()), VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), OverlayLayout.GetAddressOf()))) {
        ErrorHandler::Report("Renderer", "Gizmo pipeline allocation failed", ErrorHandler::EErrorLevel::Error);
        return false;
    }

    mMeshVertexShader = std::move(MeshVertex);
    mMeshPixelShader = std::move(MeshPixel);
    mMeshLayout = std::move(MeshLayout);
    mLineVertexShader = std::move(LineVertex);
    mLinePixelShader = std::move(LinePixel);
    mLineLayout = std::move(LineLayout);
    mGizmoVertexShader = std::move(GizmoVertex);
    mGizmoPixelShader = std::move(GizmoPixel);
    mOverlayLayout = std::move(OverlayLayout);
    mShadersDirty = false;
    return true;
}

/**
 * @brief 스왑 체인을 조정하고 백 버퍼 렌더 타깃 뷰를 생성합니다.
 */
bool FRenderer::CreateBackBuffer(Uint32 Width, Uint32 Height) {
    mContext->OMSetRenderTargets(0, nullptr, nullptr);
    mBackBuffer.reset();

    if (FAILED(mSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0))) {
        ErrorHandler::Fatal("Renderer", "Swap chain resize failed");
    }

    ComPtr<ID3D11Texture2D> Texture{};
    auto BackBuffer{NewObject<UTexture2D>()};

    if (FAILED(mSwapChain->GetBuffer(0, IID_PPV_ARGS(Texture.GetAddressOf()))) || !BackBuffer->AttachResource(Texture.Get(), ETextureUsage::RenderTarget)) {
        ErrorHandler::Fatal("Renderer", "Back buffer allocation failed");
    }

    mBackBuffer = std::move(BackBuffer);
    mWidth = Width;
    mHeight = Height;

    return true;
}

/**
 * @brief 메시·선분·기즈모 셰이더, 입력 레이아웃과 고정 파이프라인 상태를 생성합니다.
 */
bool FRenderer::CreatePipeline() {
    if (!CreateShaders()) {
        return false;
    }

    D3D11_BLEND_DESC Blend{};
    Blend.RenderTarget[0] = {true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL};
    if (FAILED(mDevice->CreateBlendState(&Blend, mOverlayBlend.GetAddressOf()))) {
        return false;
    }

    D3D11_BUFFER_DESC Buffer{};
    Buffer.ByteWidth = sizeof(FMatrix);
    Buffer.Usage = D3D11_USAGE_DEFAULT;
    Buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    if (FAILED(mDevice->CreateBuffer(&Buffer, nullptr, mViewBuffer.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "View buffer allocation failed");
    }

    Buffer.ByteWidth = sizeof(FEditorMeshConstants);
    if (FAILED(mDevice->CreateBuffer(&Buffer, nullptr, mEditorMeshBuffer.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Editor mesh constant buffer allocation failed");
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

/**
 * @brief 고정 에디터 메시의 로컬 정점과 인덱스를 종류별 불변 GPU 버퍼에 업로드합니다.
 */
bool FRenderer::CreateEditorMeshes() {
    for (std::size_t Index{}; Index < mEditorMeshes.size(); ++Index) {
        auto Resource{NewObject<UStaticMesh>()};

        if (!Resource->SetMeshData(GetEditorMesh(static_cast<EEditorMesh>(Index))) || !Resource->CreateResources(mDevice.Get())) {
            return false;
        }

        mEditorMeshes[Index] = std::move(Resource);
    }
    return true;
}

void FRenderer::DrawMeshes(const FPrimitiveDrawInterface* Elements, bool DepthTest) {
    const UINT Stride{UStaticMesh::GetPositionStride()};
    const UINT Offset{};
    mContext->IASetInputLayout(mOverlayLayout.Get());
    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->VSSetShader(mGizmoVertexShader.Get(), nullptr, 0);
    mContext->PSSetShader(mGizmoPixelShader.Get(), nullptr, 0);
    ID3D11Buffer* ConstantsBuffer{mEditorMeshBuffer.Get()};
    mContext->VSSetConstantBuffers(0, 1, &ConstantsBuffer);
    mContext->PSSetConstantBuffers(0, 1, &ConstantsBuffer);
    mContext->OMSetDepthStencilState(DepthTest ? mDepthReadOnly.Get() : mDepthEnabled.Get(), 0);
    mContext->OMSetBlendState(mOverlayBlend.Get(), nullptr, 0xFFFFFFFFu);

    for (const FDrawMesh& Draw : Elements->GetMeshes()) {
        if (Draw.mMesh >= EEditorMesh::Count || Draw.mDepthTest != DepthTest) {
            continue;
        }
        const UStaticMesh* Mesh{mEditorMeshes[static_cast<std::size_t>(Draw.mMesh)].get()};

        if (Mesh == nullptr || !Mesh->HasResources()) {
            continue;
        }

        const FEditorMeshConstants Constants{Draw.mWorld, {Draw.mColor.x, Draw.mColor.y, Draw.mColor.z, Draw.mColor.w}};
        mContext->UpdateSubresource(mEditorMeshBuffer.Get(), 0, nullptr, &Constants, 0, 0);
        ID3D11Buffer* Buffer{Mesh->GetPositionBuffer()};
        mContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
        mContext->IASetIndexBuffer(Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
        mContext->DrawIndexed(Mesh->GetIndexCount(), 0, 0);
        ++mStats.mDrawCalls;
    }

    mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
}
