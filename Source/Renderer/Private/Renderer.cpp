/**
 * @file Renderer.cpp
 * @brief Direct3D 11 렌더러와 프레임 제출을 제공합니다.
 * @ingroup Renderer
 */

#include "Renderer/Renderer.h"
#include "RendererStats.h"
#include "Core/FilePath.h"
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
#include "FMaterialPipeline.h"
#include "FLightingResources.h"
#include "FShadowResources.h"
#include "FEditorMeshConstants.h"
#include "FTextRenderer.h"
#include "FSpriteRenderer.h"
#include "Engine/EditorMesh.h"
#include "Core/Math/FVector4.h"

#include <d3dcompiler.h>
#include <dxgi1_5.h>
#include <algorithm>
#include <span>
#include <cmath>
#include <cctype>
#include <variant>

using namespace Microsoft::WRL;

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 프로젝트 또는 실행 파일의 기본 콘텐츠에서 우선순위에 맞는 경로를 반환합니다.
     * @param[in] ContentRoot 프로젝트 Content 루트입니다.
     * @param[in] RelativePath Content 기준 파일 경로입니다.
     * @param[in] EngineFirst true이면 GPU 입력 형식과 함께 배포한 엔진 셰이더를 우선 사용합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    std::filesystem::path FindTextContent(const std::filesystem::path& ContentRoot, const std::filesystem::path& RelativePath, bool EngineFirst = false) {
        std::error_code Error{};
        const std::filesystem::path ProjectPath{ContentRoot / RelativePath};
        if (!EngineFirst && std::filesystem::is_regular_file(ProjectPath, Error)) {
            return ProjectPath;
        }
        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        const std::filesystem::path EnginePath{Length > 0 && Length < Executable.size() ? std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project/Content" / RelativePath : ProjectPath};
        return EngineFirst && !std::filesystem::is_regular_file(EnginePath, Error) ? ProjectPath : EnginePath;
    }
    /**
     * @brief 메시 정점 및 픽셀 셰이더의 내장 HLSL 소스입니다.
     */
    constexpr char MeshShader[]{"Lit.hlsl"};

    /**
     * @brief 보조 선분 정점 및 픽셀 셰이더의 내장 HLSL 소스입니다.
     */
    constexpr char LineShader[]{"Line.hlsl"};

    /**
     * @brief 기즈모의 월드 행렬 변환과 조명 없는 RGBA 출력을 수행하는 내장 HLSL 소스입니다.
     */
    constexpr char GizmoShader[]{"Gizmo.hlsl"};

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

void FRenderer::SetProjectPaths(const std::filesystem::path& ContentRoot, const std::filesystem::path& CacheDirectory) {
    mShaderDirectory = ContentRoot / "Shaders";
    mShaderManager.SetCacheDirectory(CacheDirectory);
}

/**
 * @brief Direct3D 11 장치, 스왑 체인과 렌더 파이프라인을 준비합니다.
 */
bool FRenderer::Initialize(void* WindowHandle, Uint32 Width, Uint32 Height) {
    if (!InitializeUi(WindowHandle, Width, Height) || !CreatePipeline() || !CreateEditorMeshes()) {
        ErrorHandler::Fatal("Renderer", "Render pipeline creation failed");
    }
    return true;
}

bool FRenderer::InitializeUi(void* WindowHandle, Uint32 Width, Uint32 Height) {
    if (mDevice != nullptr || WindowHandle == nullptr || Width == 0 || Height == 0) {
        ErrorHandler::Fatal("Renderer", "Invalid renderer initialization state");
    }

    ComPtr<IDXGIFactory1> TearingFactory{};
    ComPtr<IDXGIFactory5> TearingFactory5{};
    BOOL TearingSupported{FALSE};
    mTearingSupported = SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(TearingFactory.GetAddressOf()))) && SUCCEEDED(TearingFactory.As(&TearingFactory5)) && SUCCEEDED(TearingFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &TearingSupported, sizeof(TearingSupported))) && TearingSupported != FALSE;

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
    Description.Flags = mTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

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

    return CreateBackBuffer(Width, Height);
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
    mTextRenderer.reset();
    mSpriteRenderer.reset();

    mOverlayLayout.Reset();
    mEditorMeshBuffer.Reset();
    mOverlayBlend.Reset();
    mViewBuffer.Reset();
    mLineViewBuffer.Reset();
    mLineBuffer.Reset();
    mMeshInstanceBuffer.Reset();

    mMaterialPipelines.clear();
    mLightingResources.reset();
    mShadowResources.reset();
    mDefaultMaterial.reset();
    mDefaultMaterialTexture.reset();
    mMaterialSampler.Reset();

    mLineVertexShader.Reset();
    mLinePixelShader.Reset();
    mLineLayout.Reset();

    mGizmoVertexShader.Reset();
    mGizmoPixelShader.Reset();
    mShaderManager.Clear();
    mShadersDirty = false;

    mRasterizer.Reset();
    mWireframeRasterizer.Reset();
    mDepthEnabled.Reset();
    mDepthReadOnly.Reset();
    mDepthDisabled.Reset();

    mBackBuffer.reset();
    mSwapChain.Reset();
    mAllowTearing = false;
    mTearingSupported = false;
    if (mContext != nullptr) {
        mContext->Flush();
    }
    mContext.Reset();
    mDevice.Reset();

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
    static FStat RenderTime{"unit", "Render submission CPU (inclusive)", EStatUnit::Milliseconds};
    static FStat Views{"render", "Rendered views"};
    static FStat DrawCalls{"render", "Scene/editor draw calls"};
    static FStat Primitives{"render", "Visible primitives (sum of views)"};
    static FStat Uploads{"render", "Object uploads"};
    static FStat GpuBytes{"memory", "GPU buffers/target estimate (last view)", EStatUnit::Bytes, EStatMode::Gauge};
    const FScopedStatTimer StatTimer{RenderTime};
    const FRenderStats PreviousStats{mStats};
    const auto* Scene{dynamic_cast<const FScene*>(Family.mScene)};
    auto* Target{dynamic_cast<FDXRenderTarget*>(Family.mTarget)};

    if (Scene == nullptr || Target == nullptr || mContext == nullptr) {
        ErrorHandler::Fatal("Renderer", "Invalid scene view family");
    }

    ID3D11ShaderResourceView* Empty{};
    mContext->PSSetShaderResources(0, 1, &Empty);

    ID3D11RenderTargetView* RenderTarget{Target->GetRenderTargetView()};
    if (mShadowResources != nullptr && !mShadowResources->Render(mDevice.Get(), mContext.Get(), *Scene, Family, mMeshes, mStats)) {
        ErrorHandler::Report("Shadow", "Shadow atlas rendering failed", ErrorHandler::EErrorLevel::Error);
    }
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

    const FSceneDrawContext Context{this, mContext.Get(), &mMeshes, mViewBuffer.Get(), &mMeshInstanceBuffer};
    FSceneRenderer SceneRenderer{Family, Scene, Context, mStats};
    SceneRenderer.Render();
    const FVector4 LineView{Family.mView.mSize.x, Family.mView.mSize.y, 1.0f / std::max(Family.mView.mSize.x, 1.0f), 1.0f / std::max(Family.mView.mSize.y, 1.0f)};
    mContext->UpdateSubresource(mLineViewBuffer.Get(), 0, nullptr, &LineView, 0, 0);

    if (Family.mEditorElements != nullptr) {
        DrawMeshes(Family.mEditorElements, true);
        DrawLines(Family.mEditorElements, true);
    }
    DrawTranslucency(Family);
    if (Family.mEditorElements != nullptr) {
        mContext->ClearDepthStencilView(Target->GetDepthView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        DrawMeshes(Family.mEditorElements, false);
        DrawLines(Family.mEditorElements, false);
    }
    DrawOverlayTexts(Family);

    mStats.mGpuBytes = Scene->GetPrimitiveCount() * sizeof(FObjectConstants) + sizeof(FMatrix) + sizeof(FEditorMeshConstants) + mLineBuffer.GetResourceBytes() + Target->GetResourceBytes();
    mStats.mGpuBytes += mTextRenderer != nullptr ? mTextRenderer->GetResourceBytes() : 0;
    mStats.mGpuBytes += mSpriteRenderer != nullptr ? mSpriteRenderer->GetResourceBytes() : 0;
    mStats.mGpuBytes += mStats.mShadowAtlasBytes;
    mStats.mGpuBytes += mMeshInstanceBuffer.GetResourceBytes();

    for (const auto& Mesh : mMeshes) {
        mStats.mGpuBytes += Mesh != nullptr ? Mesh->GetResourceBytes() : 0;
    }
    for (const auto& Mesh : mEditorMeshes) {
        mStats.mGpuBytes += Mesh != nullptr ? Mesh->GetResourceBytes() : 0;
    }

    mContext->OMSetRenderTargets(0, nullptr, nullptr);
    Views.Add();
    DrawCalls.Add(mStats.mDrawCalls - PreviousStats.mDrawCalls);
    Primitives.Add(mStats.mVisiblePrimitives - PreviousStats.mVisiblePrimitives);
    Uploads.Add(mStats.mTransformUpdates - PreviousStats.mTransformUpdates);
    GpuBytes.Set(static_cast<double>(mStats.mGpuBytes));
    RendererStats::ShadowMaps.Add(mStats.mShadowMaps);
    RendererStats::ShadowDrawCalls.Add(mStats.mShadowDrawCalls);
    RendererStats::ShadowReducedLights.Add(mStats.mShadowReducedLights);
    RendererStats::ShadowDroppedLights.Add(mStats.mShadowDroppedLights);
    RendererStats::ShadowAtlasBytes.Set(static_cast<double>(mStats.mShadowAtlasBytes));
    RendererStats::ShadowAllocatedBytes.Set(static_cast<double>(mStats.mShadowAllocatedBytes));
    RendererStats::DirectionalLights.Add(mStats.mDirectionalLights);
    RendererStats::PointLights.Add(mStats.mPointLights);
    RendererStats::SpotLights.Add(mStats.mSpotLights);
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
    static FStat PresentTime{"unit", "Present / VSync CPU", EStatUnit::Milliseconds};
    const FScopedStatTimer StatTimer{PresentTime};
    if (mSwapChain == nullptr) {
        return;
    }

    BOOL Fullscreen{FALSE};
    const bool AllowTearing{mAllowTearing && mTearingSupported && SUCCEEDED(mSwapChain->GetFullscreenState(&Fullscreen, nullptr)) && Fullscreen == FALSE};
    const UINT SyncInterval{AllowTearing ? 0u : 1u};
    const UINT Flags{AllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0u};
    if (FAILED(mSwapChain->Present(SyncInterval, Flags))) {
        ErrorHandler::Fatal("Renderer", "Presentation failed");
    }
}

bool FRenderer::SetAllowTearing(bool Allow) {
    if (mSwapChain == nullptr || (Allow && !mTearingSupported)) {
        return false;
    }
    mAllowTearing = Allow;
    return true;
}

bool FRenderer::IsTearingAllowed() const {
    return mAllowTearing;
}

bool FRenderer::IsTearingSupported() const {
    return mTearingSupported;
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

FShadowAtlasView FRenderer::GetShadowAtlasView() const {
    return mShadowResources != nullptr ? mShadowResources->GetView() : FShadowAtlasView{};
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
        ErrorHandler::Report("Shader", Diagnostics, ErrorHandler::EErrorLevel::Warning);
    }

    bool PipelinesReady{true};
    if (Success) {
        for (auto Iterator{mMaterialPipelines.begin()}; Iterator != mMaterialPipelines.end();) {
            if (Iterator->second == nullptr) {
                Iterator = mMaterialPipelines.erase(Iterator);
                continue;
            }
            FString Error{};
            if (!Iterator->second->IsCurrent() && !Iterator->second->CreateResources(mDevice.Get(), Error)) {
                ErrorHandler::Report("Material", Error, ErrorHandler::EErrorLevel::Warning);
                PipelinesReady = false;
            }
            ++Iterator;
        }
    }
    return Success && (!mShadersDirty || CreateShaders()) && PipelinesReady;
}

bool FRenderer::GetTextBounds(const FString& Text, FVector2& Minimum, FVector2& Maximum) const {
    return mTextRenderer != nullptr && mTextRenderer->GetBounds(Text, Minimum, Maximum);
}

/**
 * @brief 내장 HLSL 소스를 지정한 진입점과 프로파일로 컴파일합니다.
 */
bool FRenderer::Compile(const char* Source, const char* Entry, const char* Target, std::shared_ptr<const FShader>& Code) {
    FShaderDescription Description{};
    const bool EngineFirst{std::string_view{Source} == LineShader || std::string_view{Source} == "Sprite.hlsl"};
    Description.mSourcePath = FindTextContent(mShaderDirectory.parent_path(), std::filesystem::path{"Shaders"} / Source, EngineFirst);
    Description.mIncludeDirectories = {mShaderDirectory};
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
    ComPtr<ID3D11VertexShader> LineVertex{};
    ComPtr<ID3D11PixelShader> LinePixel{};
    ComPtr<ID3D11InputLayout> LineLayout{};
    ComPtr<ID3D11VertexShader> GizmoVertex{};
    ComPtr<ID3D11PixelShader> GizmoPixel{};
    ComPtr<ID3D11InputLayout> OverlayLayout{};

    if (!Compile(LineShader, "VertexMain", "vs_5_0", VertexCode) || !Compile(LineShader, "PixelMain", "ps_5_0", PixelCode)) {
        return false;
    }

    const std::array<D3D11_INPUT_ELEMENT_DESC, 3> LineElements{{{"START", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FLineVertex, mStartAndWidth), D3D11_INPUT_PER_INSTANCE_DATA, 1}, {"END", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FLineVertex, mEnd), D3D11_INPUT_PER_INSTANCE_DATA, 1}, {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FLineVertex, mColor), D3D11_INPUT_PER_INSTANCE_DATA, 1}}};

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

    if (!Compile("Text.hlsl", "VertexMain", "vs_5_0", VertexCode) || !Compile("Text.hlsl", "PixelMain", "ps_5_0", PixelCode) || !mTextRenderer->CreateShaders(mDevice.Get(), VertexCode->GetByteCode(), PixelCode->GetByteCode())) {
        return false;
    }
    if (!Compile("Sprite.hlsl", "VertexMain", "vs_5_0", VertexCode) || !Compile("Sprite.hlsl", "PixelMain", "ps_5_0", PixelCode) || !mSpriteRenderer->CreateShaders(mDevice.Get(), VertexCode->GetByteCode(), PixelCode->GetByteCode())) {
        return false;
    }

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

    const UINT Flags{mTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u};
    if (FAILED(mSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, Flags))) {
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
    mLightingResources = std::make_unique<FLightingResources>();
    mShadowResources = std::make_unique<FShadowResources>();
    mTextRenderer = std::make_unique<FTextRenderer>();
    mSpriteRenderer = std::make_unique<FSpriteRenderer>();
    if (!mSpriteRenderer->Initialize(mDevice.Get())) {
        return false;
    }
    if (!mTextRenderer->Initialize(mDevice.Get(), FindTextContent(mShaderDirectory.parent_path(), "Fonts/Default.json"))) {
        return false;
    }
    if (!CreateShaders()) {
        return false;
    }

    mDefaultMaterial = NewObject<UMaterial>();
    mDefaultMaterial->SetShaderPath(FString{"Shaders/"} + MeshShader);
    mDefaultMaterialTexture = NewObject<UTexture2D>();
    if (!mDefaultMaterialTexture->SetPixels(1, 1, ETextureFormat::RGBA8, TArray<Uint8>{255, 255, 255, 255}) || !mDefaultMaterialTexture->CreateResources(mDevice.Get()) || GetMaterialPipeline(*mDefaultMaterial) == nullptr) {
        return false;
    }
    D3D11_SAMPLER_DESC Sampler{};
    Sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    Sampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    Sampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    Sampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    Sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(mDevice->CreateSamplerState(&Sampler, mMaterialSampler.GetAddressOf()))) {
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
    Buffer.ByteWidth = sizeof(FVector4);
    if (FAILED(mDevice->CreateBuffer(&Buffer, nullptr, mLineViewBuffer.GetAddressOf()))) {
        return false;
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
    Rasterizer.FillMode = D3D11_FILL_WIREFRAME;
    if (FAILED(mDevice->CreateRasterizerState(&Rasterizer, mWireframeRasterizer.GetAddressOf()))) {
        return false;
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
 * @brief 월드 선은 깊이를 기록하고 편집 오버레이 선은 깊이 검사 없이 그립니다.
 */
void FRenderer::DrawLines(const FPrimitiveDrawInterface* Elements, bool DepthTest) {
    mContext->RSSetState(mRasterizer.Get());
    mLineVertices.clear();

    for (const FDrawLine& Line : Elements->GetLines()) {
        if (Line.mDepthTest == DepthTest) {
            mLineVertices.emplace_back(Line);
        }
    }

    if (mLineVertices.empty()) {
        return;
    }

    const Uint32 Count{static_cast<Uint32>(mLineVertices.size())};

    FString Error{};
    if (!mLineBuffer.Upload(mContext.Get(), std::as_bytes(std::span{mLineVertices}), sizeof(FLineVertex), Count + 256, Error)) {
        ErrorHandler::Fatal("Renderer", Error);
        return;
    }

    const UINT Stride{sizeof(FLineVertex)};
    const UINT Offset{};
    ID3D11Buffer* Buffer{mLineBuffer.GetBuffer()};

    mContext->IASetInputLayout(mLineLayout.Get());
    mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);

    mContext->VSSetShader(mLineVertexShader.Get(), nullptr, 0);
    mContext->PSSetShader(mLinePixelShader.Get(), nullptr, 0);
    ID3D11Buffer* Constants{mLineViewBuffer.Get()};
    ID3D11Buffer* ViewBuffer{mViewBuffer.Get()};
    mContext->VSSetConstantBuffers(0, 1, &Constants);
    mContext->VSSetConstantBuffers(1, 1, &ViewBuffer);
    mContext->OMSetBlendState(mOverlayBlend.Get(), nullptr, 0xFFFFFFFFu);
    mContext->OMSetDepthStencilState(DepthTest ? mDepthEnabled.Get() : mDepthDisabled.Get(), 0);

    mContext->DrawInstanced(6, Count, 0, 0);
    RendererStats::Triangles.Add(static_cast<double>(Count) * 2.0);
    mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);

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
    mContext->RSSetState(mRasterizer.Get());
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
        RendererStats::Triangles.Add(Mesh->GetIndexCount() / 3);
        ++mStats.mDrawCalls;
    }

    mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
}

void FRenderer::DrawOverlayTexts(const FSceneViewFamily& Family) {
    mContext->RSSetState(mRasterizer.Get());
    if (mTextRenderer == nullptr || !Family.mShowFlags.mText) {
        return;
    }
    TArray<FDrawText> Texts{};
    if (Family.mEditorElements != nullptr) {
        for (const FDrawText& Text : Family.mEditorElements->GetTexts()) {
            if (!Text.mDepthTest) {
                Texts.push_back(Text);
            }
        }
    }
    std::stable_sort(Texts.begin(), Texts.end(), [&Family](const FDrawText& First, const FDrawText& Second) {
        if (First.mScreenSpace != Second.mScreenSpace) {
            return !First.mScreenSpace;
        }
        if (First.mScreenSpace) {
            return First.mPosition.z > Second.mPosition.z;
        }
        return (First.mPosition - Family.mView.mLocation).Dot(Family.mView.mForward) > (Second.mPosition - Family.mView.mLocation).Dot(Family.mView.mForward);
    });
    mContext->OMSetBlendState(mOverlayBlend.Get(), nullptr, 0xFFFFFFFFu);
    mContext->OMSetDepthStencilState(mDepthDisabled.Get(), 0);
    for (const FDrawText& Text : Texts) {
        if (mTextRenderer->Draw(mContext.Get(), Text, Family.mView)) {
            ++mStats.mDrawCalls;
        }
    }
    mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
}

FMaterialPipeline* FRenderer::GetMaterialPipeline(const UMaterial& Material, EViewMode Mode) {
    const FString& Relative{Material.GetShaderPath()};
    const std::filesystem::path RelativePath{FilePath::FromUtf8(Relative).lexically_normal()};
    FString Key{FilePath::ToGenericUtf8(RelativePath) + (Material.IsTwoSided() ? "|TwoSided" : "|OneSided")};
    std::transform(Key.begin(), Key.end(), Key.begin(), [](unsigned char Character) {
        return static_cast<char>(std::tolower(Character));
    });
    Key += "|" + std::to_string(static_cast<Uint32>(Mode)) + (Material.GetNormalMapTexture() != nullptr ? "|NormalMap" : "|VertexNormal");
    const auto Existing{mMaterialPipelines.find(Key)};
    if (Existing != mMaterialPipelines.end()) {
        return Existing->second.get();
    }
    if (mShadersDirty) {
        return nullptr;
    }
    const bool Builtin{RelativePath == "Shaders/Lit.hlsl" || RelativePath == "Shaders/Unlit.hlsl" || RelativePath == "Shaders/Mesh.hlsl" || Mode == EViewMode::Unlit || Mode == EViewMode::WorldNormal};
    const std::filesystem::path Source{FindTextContent(mShaderDirectory.parent_path(), Mode == EViewMode::Unlit || Mode == EViewMode::WorldNormal ? std::filesystem::path{"Shaders/Lit.hlsl"} : RelativePath, Builtin)};
    FShaderDescription Description{};
    Description.mSourcePath = Source;
    Description.mIncludeDirectories = {Source.parent_path(), mShaderDirectory};
    const char* Model{Mode == EViewMode::Lit_Gouraud ? "LIGHTING_MODEL_GOURAUD" : Mode == EViewMode::Lit_Lambert ? "LIGHTING_MODEL_LAMBERT" : "LIGHTING_MODEL_PHONG"};
    Description.mMacros.push_back({Model, "1"});
    if (Builtin) {
        Description.mMacros.push_back({"MacawInstanced", "1"});
    }
    if (Mode == EViewMode::Unlit) {
        Description.mMacros.push_back({"MACAW_UNLIT", "1"});
    }
    if (Mode == EViewMode::WorldNormal) {
        Description.mMacros.push_back({"WORLD_NORMAL", "1"});
    }
    if (Material.GetNormalMapTexture() != nullptr) {
        Description.mMacros.push_back({"HAS_NORMAL_MAP", "1"});
    }
    Description.mEntryPoint = "VertexMain";
    Description.mTargetProfile = "vs_5_0";
#ifdef _DEBUG
    Description.mCompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    FString Diagnostics{};
    std::shared_ptr<const FShader> Vertex{mShaderManager.Load(Description, Diagnostics)};
    FString Error{Diagnostics};
    Description.mEntryPoint = "PixelMain";
    Description.mTargetProfile = "ps_5_0";
    std::shared_ptr<const FShader> Pixel{mShaderManager.Load(Description, Diagnostics)};
    Error += Diagnostics;
    std::unique_ptr<FMaterialPipeline> Pipeline{};
    if (Vertex != nullptr && Pixel != nullptr) {
        Pipeline = std::make_unique<FMaterialPipeline>(std::move(Vertex), std::move(Pixel), Material.IsTwoSided(), Builtin);
        if (!Pipeline->CreateResources(mDevice.Get(), Diagnostics)) {
            Error += Diagnostics;
            Pipeline.reset();
        }
    }
    if (!Error.empty()) {
        ErrorHandler::Report("Material", Error, ErrorHandler::EErrorLevel::Warning);
    }
    FMaterialPipeline* Result{Pipeline.get()};
    mMaterialPipelines.emplace(std::move(Key), std::move(Pipeline));
    return Result;
}

UMaterial* FRenderer::GetDefaultMaterial() const {
    return mDefaultMaterial.get();
}

ID3D11ShaderResourceView* FRenderer::GetDefaultMaterialTexture() const {
    return mDefaultMaterialTexture->GetShaderResourceView();
}

ID3D11SamplerState* FRenderer::GetMaterialSampler() const {
    return mMaterialSampler.Get();
}

ID3D11RasterizerState* FRenderer::GetWireframeRasterizer() const {
    return mWireframeRasterizer.Get();
}

void FRenderer::DrawTranslucency(const FSceneViewFamily& Family) {
    using FPrimitive = std::variant<const FPrimitiveSceneProxy*, FDrawText>;
    using FCommand = std::pair<float, FPrimitive>;
    TArray<FCommand> Commands{};
    const auto Depth{[&Family](const FVector& Position, bool ScreenSpace) {
        const FVector4 Clip{FVector4{Position, 1.0f} * Family.mView.mViewProjection};
        const float Value{ScreenSpace ? Position.z : Clip.w > 0.0f ? Clip.z / Clip.w : -1.0f};
        return std::isfinite(Value) ? Value : -1.0f;
    }};
    TArray<FPrimitiveSceneProxy> LightIcons{};
    if (Family.mEditorElements != nullptr && Family.mShowFlags.mLightIcons) {
        const auto* Scene{static_cast<const FScene*>(Family.mScene)};
        LightIcons.reserve(Scene->GetLights().size());
        for (const FLightSceneProxy& Light : Scene->GetLights()) {
            LightIcons.push_back(Light.CreateIconProxy());
        }
        for (const FPrimitiveSceneProxy& Icon : LightIcons) {
            Commands.emplace_back(Depth(Icon.mWorld.TransformPosition({}), false), FPrimitive{&Icon});
        }
    }
    if (Family.mShowFlags.mPrimitives) {
        const auto* Scene{static_cast<const FScene*>(Family.mScene)};
        for (const FScenePrimitive& Primitive : Scene->GetPrimitives()) {
            const FPrimitiveSceneProxy& Proxy{Primitive.mProxy};
            if (Proxy.mSprite.has_value() && Family.mShowFlags.mSprites) {
                Commands.emplace_back(Depth(Proxy.mWorld.TransformPosition({}), false), FPrimitive{&Proxy});
            } else if (Proxy.mText.has_value() && Family.mShowFlags.mText) {
                FDrawText Text{*Proxy.mText};
                Text.mPosition = Proxy.mWorld.TransformPosition(Text.mPosition);
                Text.mScale.x *= Proxy.mWorld.TransformVector({0.0f, 1.0f, 0.0f}).Length();
                Text.mScale.y *= Proxy.mWorld.TransformVector({0.0f, 0.0f, 1.0f}).Length();
                const float Value{Depth(Text.mPosition, Text.mScreenSpace)};
                Commands.emplace_back(Value, FPrimitive{std::move(Text)});
            }
        }
    }
    if (Family.mEditorElements != nullptr && Family.mShowFlags.mText) {
        for (const FDrawText& Text : Family.mEditorElements->GetTexts()) {
            if (Text.mDepthTest) {
                Commands.emplace_back(Depth(Text.mPosition, Text.mScreenSpace), FPrimitive{Text});
            }
        }
    }
    std::stable_sort(Commands.begin(), Commands.end(), [](const FCommand& First, const FCommand& Second) {
        return First.first > Second.first;
    });
    mContext->OMSetBlendState(mOverlayBlend.Get(), nullptr, 0xFFFFFFFFu);
    mContext->OMSetDepthStencilState(mDepthReadOnly.Get(), 0);
    for (const FCommand& Command : Commands) {
        bool Drawn{};
        if (const auto* Sprite{std::get_if<const FPrimitiveSceneProxy*>(&Command.second)}) {
            mContext->RSSetState(Family.mViewMode == EViewMode::Wireframe ? mWireframeRasterizer.Get() : mRasterizer.Get());
            Drawn = mSpriteRenderer != nullptr && mSpriteRenderer->Draw(mContext.Get(), **Sprite, Family.mView);
        } else {
            mContext->RSSetState(mRasterizer.Get());
            Drawn = mTextRenderer != nullptr && mTextRenderer->Draw(mContext.Get(), std::get<FDrawText>(Command.second), Family.mView);
        }
        if (Drawn) {
            ++mStats.mDrawCalls;
            ++mStats.mVisiblePrimitives;
        }
    }
    mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
}

bool FRenderer::PrepareLighting(const FSceneViewFamily& Family) {
    const auto* Scene{static_cast<const FScene*>(Family.mScene)};
    if (mShadowResources != nullptr) {
        mShadowResources->Bind(mContext.Get());
    }
    return mLightingResources != nullptr && mLightingResources->Upload(mDevice.Get(), mContext.Get(), Scene->GetLights(), Family.mView, mShadowResources != nullptr ? std::span<const std::array<Uint32, 2>>{mShadowResources->GetLightRanges()} : std::span<const std::array<Uint32, 2>>{});
}

void FRenderer::FinishLighting() {
    if (mShadowResources != nullptr) {
        mShadowResources->Unbind(mContext.Get());
    }
    if (mLightingResources != nullptr) {
        mLightingResources->Unbind(mContext.Get());
    }
}
