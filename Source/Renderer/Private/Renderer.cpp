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
#include "Asset/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FDrawLine.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/FRenderStats.h"
#include "FObjectConstants.h"
#include "Asset/UStaticMesh.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Base/ErrorHandler.h"
#include "RenderTarget.h"
#include "FScene.h"
#include "FMeshPassParameters.h"
#include "FMeshDrawCommand.h"
#include "FShadowDepthPass.h"
#include "FBasePass.h"
#include "FMaterialPipeline.h"
#include "FLightingResources.h"
#include "FShadowResources.h"
#include "FPostProcessResources.h"
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

namespace Renderer {

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
    Renderer::FRenderer::FRenderer() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Renderer::FRenderer::~FRenderer() {
        Shutdown();
    }

    void Renderer::FRenderer::SetProjectPaths(const std::filesystem::path& ContentRoot, const std::filesystem::path& CacheDirectory) {
        mShaderDirectory = ContentRoot / "Shaders";
        mShaderManager.SetCacheDirectory(CacheDirectory);
    }

    /**
     * @brief Direct3D 11 장치, 스왑 체인과 렌더 파이프라인을 준비합니다.
     */
    bool Renderer::FRenderer::Initialize(void* WindowHandle, Core::Uint32 Width, Core::Uint32 Height) {
        if (!InitializeUi(WindowHandle, Width, Height) || !CreatePipeline() || !CreateEditorMeshes()) {
            Core::ErrorHandler::Fatal("Renderer", "Render pipeline creation failed");
        }

        return true;
    }

    bool Renderer::FRenderer::InitializeUi(void* WindowHandle, Core::Uint32 Width, Core::Uint32 Height) {
        if (mDevice != nullptr || WindowHandle == nullptr || Width == 0 || Height == 0) {
            Core::ErrorHandler::Fatal("Renderer", "Invalid renderer initialization state");
        }

        Microsoft::WRL::ComPtr<IDXGIFactory1> TearingFactory{};
        Microsoft::WRL::ComPtr<IDXGIFactory5> TearingFactory5{};
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

        for (Core::Int32 Attempt{}; Attempt < 4 && FAILED(Result); ++Attempt) {
            const D3D_DRIVER_TYPE Driver{Attempt >= 2 ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE};
            const UINT AttemptFlags{(Attempt & 1) != 0 ? Flags & ~D3D11_CREATE_DEVICE_DEBUG : Flags};

            Result = D3D11CreateDeviceAndSwapChain(nullptr, Driver, nullptr, AttemptFlags, &Level, 1, D3D11_SDK_VERSION, &Description, mSwapChain.GetAddressOf(), mDevice.GetAddressOf(), nullptr, mContext.GetAddressOf());
        }

        if (FAILED(Result)) {
            Core::ErrorHandler::ReportHRESULT(Result, "Renderer", "D3D11 device creation failed", Core::ErrorHandler::EErrorLevel::Critical);
        }

        Microsoft::WRL::ComPtr<IDXGIFactory> Factory{};

        if (SUCCEEDED(mSwapChain->GetParent(IID_PPV_ARGS(Factory.GetAddressOf())))) {
            Factory->MakeWindowAssociation(Description.OutputWindow, DXGI_MWA_NO_ALT_ENTER);
        }

        return CreateBackBuffer(Width, Height);
    }

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Renderer::FRenderer::Shutdown() {
        if (mContext != nullptr) {
            mContext->ClearState();
            mContext->Flush();
        }

        mMeshes = {};
        mEditorMeshes = {};
        mLineVertices = Core::TArray<Renderer::FLineVertex>{};
        mTextRenderer.reset();
        mSpriteRenderer.reset();
        mPostProcessResources.reset();

        mOverlayLayout.Reset();
        mEditorMeshBuffer.Reset();
        mOverlayBlend.Reset();
        mViewBuffer.Reset();
        mLineViewBuffer.Reset();
        mLineBuffer.Reset();
        mMeshInstanceBuffer.Reset();

        mShadowDepthPass.reset();
        mBasePass.reset();
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
        mDepthClearVertexShader.Reset();
        mShaderManager.Clear();
        mShadersDirty = false;

        mRasterizer.Reset();
        mWireframeRasterizer.Reset();
        mScissorRasterizer.Reset();
        mDepthEnabled.Reset();
        mDepthReadOnly.Reset();
        mDepthDisabled.Reset();
        mDepthAlways.Reset();

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
    std::unique_ptr<Engine::FSceneInterface> Renderer::FRenderer::AllocateScene() {
        Core::ErrorHandler::Report(mDevice == nullptr, "Renderer", "Scene allocation requires an initialized device", Core::ErrorHandler::EErrorLevel::Critical);

        return std::make_unique<Renderer::FScene>(mDevice.Get(), mContext.Get());
    }

    /**
     * @brief CPU 메시를 종류별 GPU 정점·인덱스 버퍼에 업로드합니다.
     */
    bool Renderer::FRenderer::UploadMesh(Engine::EPrimitiveType Type, const Asset::FMeshData& Mesh) {
        if (Type >= Engine::EPrimitiveType::Count || mDevice == nullptr) {
            return false;
        }

        auto Resource{CoreUObject::NewObject<Asset::UStaticMesh>()};

        if (!Resource->SetMeshData(Mesh) || !Resource->CreateResources(mDevice.Get())) {
            return false;
        }

        mMeshes[static_cast<std::size_t>(Type)] = std::move(Resource);

        return true;
    }

    /**
     * @brief 지정한 픽셀 크기의 오프스크린 렌더 타깃을 생성합니다.
     */
    std::unique_ptr<Engine::FRenderTarget> Renderer::FRenderer::CreateRenderTarget(Core::Uint32 Width, Core::Uint32 Height) {
        auto Target{std::make_unique<Renderer::FDXRenderTarget>()};
        return mDevice != nullptr && Target->Resize(mDevice.Get(), Width, Height) ? std::move(Target) : nullptr;
    }

    /**
     * @brief 기존 오프스크린 렌더 타깃을 새 픽셀 크기로 변경합니다.
     */
    bool Renderer::FRenderer::ResizeRenderTarget(Engine::FRenderTarget* Target, Core::Uint32 Width, Core::Uint32 Height) {
        auto* Native{dynamic_cast<Renderer::FDXRenderTarget*>(Target)};

        if (Native == nullptr || mDevice == nullptr) {
            Core::ErrorHandler::Fatal("Renderer", "Invalid render target resize state");
        }

        mContext->OMSetRenderTargets(0, nullptr, nullptr);

        ID3D11ShaderResourceView* Empty{};
        mContext->PSSetShaderResources(0, 1, &Empty);

        return Native->Resize(mDevice.Get(), Width, Height);
    }

    /**
     * @brief 백 버퍼 크기를 동기화하고 프레임 렌더링 통계를 초기화합니다.
     */
    void Renderer::FRenderer::BeginFrame(Core::Uint32 Width, Core::Uint32 Height) {
        mStats = {};

        if (mShadowResources != nullptr) {
            mShadowResources->BeginFrame();
        }

        if (Width > 0 && Height > 0 && (mWidth != Width || mHeight != Height)) {
            CreateBackBuffer(Width, Height);
        }
    }

    /**
     * @brief 지정한 장면 뷰를 타깃에 렌더링하고 에디터 보조 선분을 그립니다.
     */
    void Renderer::FRenderer::BeginRenderingViewFamily(const Engine::FSceneViewFamily& Family) {
        static Core::FStat RenderTime{"unit", "Render submission CPU (inclusive)", Core::EStatUnit::Milliseconds};
        static Core::FStat Views{"render", "Rendered views"};
        static Core::FStat DrawCalls{"render", "Scene/editor draw calls"};
        static Core::FStat Primitives{"render", "Visible primitives (sum of views)"};
        static Core::FStat Uploads{"render", "Object uploads"};
        static Core::FStat GpuBytes{"memory", "GPU buffers/target estimate (last view)", Core::EStatUnit::Bytes, Core::EStatMode::Gauge};
        const Core::FScopedStatTimer StatTimer{RenderTime};
        const RenderCore::FRenderStats PreviousStats{mStats};
        const auto* Scene{dynamic_cast<const Renderer::FScene*>(Family.mScene)};
        auto* Target{dynamic_cast<Renderer::FDXRenderTarget*>(Family.mTarget)};

        if (Scene == nullptr || Target == nullptr || mContext == nullptr) {
            Core::ErrorHandler::Fatal("Renderer", "Invalid scene view family");
        }

        ID3D11ShaderResourceView* Empty{};
        mContext->PSSetShaderResources(0, 1, &Empty);

        mContext->GSSetShader(nullptr, nullptr, 0);
        mContext->HSSetShader(nullptr, nullptr, 0);
        mContext->DSSetShader(nullptr, nullptr, 0);

        if (mShadowResources != nullptr) {
            if (!mShadowResources->Prepare(mDevice.Get(), mContext.Get(), *Scene, Family, mStats)) {
                Core::ErrorHandler::Report("Shadow", "Shadow atlas preparation failed", Core::ErrorHandler::EErrorLevel::Error);
            }

            DrawShadowDepths(*Scene, Family);
            mShadowResources->Finish(mContext.Get(), Family);
        }

        constexpr float ClearColor[]{0.065f, 0.078f, 0.10f, 1.0f};
        Renderer::FMeshPassParameters BaseParameters{};
        BaseParameters.mTarget.mColor = Target->GetSceneColor()->GetRenderTargetView();
        BaseParameters.mTarget.mDepth = Target->GetDepthView();
        BaseParameters.mTarget.mViewport = {0.0f, 0.0f, static_cast<float>(Target->GetWidth()), static_cast<float>(Target->GetHeight()), 0.0f, 1.0f};
        BaseParameters.mViewProjection = Family.mView.mViewProjection;
        ClearTarget(BaseParameters.mTarget, ClearColor);
        DrawMeshPass(*mBasePass, *Scene, Family, BaseParameters);

        ID3D11RenderTargetView* RenderTarget{};

        const Core::FVector4 LineView{Family.mView.mSize.x, Family.mView.mSize.y, 1.0f / std::max(Family.mView.mSize.x, 1.0f), 1.0f / std::max(Family.mView.mSize.y, 1.0f)};
        mContext->UpdateSubresource(mLineViewBuffer.Get(), 0, nullptr, &LineView, 0, 0);

        if (Family.mViewMode != Engine::EViewMode::WorldNormal && Family.mViewMode != Engine::EViewMode::SceneDepth) {
            DrawTranslucency(Family, false);
        }

        mStats.mDrawCalls += mPostProcessResources->Render(mContext.Get(), *Target, Family, Scene->GetHeightFogs());
        RenderTarget = Target->GetRenderTargetView();
        mContext->OMSetRenderTargets(1, &RenderTarget, Target->GetDepthView());
        DrawTranslucency(Family, true);

        if (Family.mEditorElements != nullptr) {
            DrawMeshes(Family.mEditorElements, true);
            DrawLines(Family.mEditorElements, true);
        }

        if (Family.mEditorElements != nullptr) {
            mContext->ClearDepthStencilView(Target->GetDepthView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
            DrawMeshes(Family.mEditorElements, false);
            DrawLines(Family.mEditorElements, false);
        }

        DrawOverlayTexts(Family);

        mStats.mGpuBytes = Scene->GetPrimitiveCount() * sizeof(Renderer::FObjectConstants) + sizeof(Core::FMatrix) + sizeof(Renderer::FEditorMeshConstants) + mLineBuffer.GetResourceBytes() + Target->GetResourceBytes();
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
        Renderer::RendererStats::ShadowMaps.Add(mStats.mShadowMaps);
        Renderer::RendererStats::ShadowDrawCalls.Add(mStats.mShadowDrawCalls);
        Renderer::RendererStats::ShadowReducedLights.Add(mStats.mShadowReducedLights);
        Renderer::RendererStats::ShadowDroppedLights.Add(mStats.mShadowDroppedLights);
        Renderer::RendererStats::ShadowAtlasBytes.Set(static_cast<double>(mStats.mShadowAtlasBytes));
        Renderer::RendererStats::ShadowAllocatedBytes.Set(static_cast<double>(mStats.mShadowAllocatedBytes));
        Renderer::RendererStats::DirectionalLights.Add(mStats.mDirectionalLights);
        Renderer::RendererStats::PointLights.Add(mStats.mPointLights);
        Renderer::RendererStats::SpotLights.Add(mStats.mSpotLights);
    }

    /**
     * @brief UI 렌더링에 사용할 백 버퍼와 뷰포트를 설정합니다.
     */
    void Renderer::FRenderer::BeginUiRender() {
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
    void Renderer::FRenderer::Present() {
        static Core::FStat PresentTime{"unit", "Present / VSync CPU", Core::EStatUnit::Milliseconds};
        const Core::FScopedStatTimer StatTimer{PresentTime};

        if (mSwapChain == nullptr) {
            return;
        }

        BOOL Fullscreen{FALSE};
        const bool AllowTearing{mAllowTearing && mTearingSupported && SUCCEEDED(mSwapChain->GetFullscreenState(&Fullscreen, nullptr)) && Fullscreen == FALSE};
        const UINT SyncInterval{AllowTearing ? 0u : 1u};
        const UINT Flags{AllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0u};

        if (FAILED(mSwapChain->Present(SyncInterval, Flags))) {
            Core::ErrorHandler::Fatal("Renderer", "Presentation failed");
        }
    }

    /**
     * @brief 렌더 타깃의 최종 컬러를 스왑 체인 백 버퍼에 복사합니다.
     */
    bool Renderer::FRenderer::CopyToBackBuffer(Engine::FRenderTarget* Target) {
        const auto* Native{dynamic_cast<Renderer::FDXRenderTarget*>(Target)};

        if (Native == nullptr || Native->GetOutputColor() == nullptr || mBackBuffer == nullptr || Native->GetWidth() != mWidth || Native->GetHeight() != mHeight) {
            return false;
        }

        ID3D11ShaderResourceView* Empty{};
        mContext->PSSetShaderResources(0, 1, &Empty);
        mContext->OMSetRenderTargets(0, nullptr, nullptr);
        mContext->CopyResource(mBackBuffer->GetTexture(), Native->GetOutputColor()->GetTexture());

        return true;
    }

    bool Renderer::FRenderer::SetAllowTearing(bool Allow) {
        if (mSwapChain == nullptr || (Allow && !mTearingSupported)) {
            return false;
        }

        mAllowTearing = Allow;

        return true;
    }

    bool Renderer::FRenderer::IsTearingAllowed() const {
        return mAllowTearing;
    }

    bool Renderer::FRenderer::IsTearingSupported() const {
        return mTearingSupported;
    }

    /**
     * @brief Direct3D 11 장치의 비소유 주소를 반환합니다.
     */
    void* Renderer::FRenderer::GetNativeDevice() const {
        return mDevice.Get();
    }

    /**
     * @brief Direct3D 11 즉시 컨텍스트의 비소유 주소를 반환합니다.
     */
    void* Renderer::FRenderer::GetNativeContext() const {
        return mContext.Get();
    }

    /**
     * @brief 프레임 렌더링 통계를 반환합니다.
     */
    RenderCore::FRenderStats Renderer::FRenderer::GetStats() const {
        return mStats;
    }

    RenderCore::FShadowAtlasView Renderer::FRenderer::GetShadowAtlasView() const {
        return mShadowResources != nullptr ? mShadowResources->GetView() : RenderCore::FShadowAtlasView{};
    }

    bool Renderer::FRenderer::ReloadChangedShaders() {
        if (mDevice == nullptr) {
            return false;
        }

        Core::TArray<std::shared_ptr<const RenderCore::FShader>> ChangedShaders{};
        Core::FString Diagnostics{};
        const bool Success{mShaderManager.ReloadChanged(ChangedShaders, Diagnostics)};
        mShadersDirty = mShadersDirty || !ChangedShaders.empty();

        if (!Diagnostics.empty()) {
            Core::ErrorHandler::Report("Shader", Diagnostics, Core::ErrorHandler::EErrorLevel::Warning);
        }

        bool PipelinesReady{true};

        if (Success) {
            for (auto Iterator{mMaterialPipelines.begin()}; Iterator != mMaterialPipelines.end();) {
                if (Iterator->second == nullptr) {
                    Iterator = mMaterialPipelines.erase(Iterator);
                    continue;
                }

                Core::FString Error{};

                if (!Iterator->second->IsCurrent() && !Iterator->second->CreateResources(mDevice.Get(), Error)) {
                    Core::ErrorHandler::Report("Material", Error, Core::ErrorHandler::EErrorLevel::Warning);
                    PipelinesReady = false;
                }

                ++Iterator;
            }
        }

        return Success && (!mShadersDirty || CreateShaders()) && PipelinesReady;
    }

    bool Renderer::FRenderer::GetTextBounds(const Core::FString& Text, Core::FVector2& Minimum, Core::FVector2& Maximum) const {
        return mTextRenderer != nullptr && mTextRenderer->GetBounds(Text, Minimum, Maximum);
    }

    /**
     * @brief 내장 HLSL 소스를 지정한 진입점과 프로파일로 컴파일합니다.
     */
    bool Renderer::FRenderer::Compile(const char* Source, const char* Entry, const char* Target, std::shared_ptr<const RenderCore::FShader>& Code) {
        RenderCore::FShaderDescription Description{};
        const bool EngineFirst{std::string_view{Source} == LineShader || std::string_view{Source} == "Sprite.hlsl" || std::string_view{Source} == "PostProcess.hlsl" || std::string_view{Source} == "Depth.hlsl"};
        Description.mSourcePath = FindTextContent(mShaderDirectory.parent_path(), std::filesystem::path{"Shaders"} / Source, EngineFirst);
        Description.mIncludeDirectories = {mShaderDirectory};
        Description.mEntryPoint = Entry;
        Description.mTargetProfile = Target;

#ifdef _DEBUG
        Description.mCompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        Core::FString Diagnostics{};
        Code = mShaderManager.Load(Description, Diagnostics);

        if (!Diagnostics.empty()) {
            Core::ErrorHandler::Report("Shader", Diagnostics, Code != nullptr ? Core::ErrorHandler::EErrorLevel::Warning : Core::ErrorHandler::EErrorLevel::Error);
        }

        return Code != nullptr;
    }

    bool Renderer::FRenderer::CreateShaders() {
        std::shared_ptr<const RenderCore::FShader> VertexCode{};
        std::shared_ptr<const RenderCore::FShader> PixelCode{};
        Microsoft::WRL::ComPtr<ID3D11VertexShader> LineVertex{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> LinePixel{};
        Microsoft::WRL::ComPtr<ID3D11InputLayout> LineLayout{};
        Microsoft::WRL::ComPtr<ID3D11VertexShader> GizmoVertex{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> GizmoPixel{};
        Microsoft::WRL::ComPtr<ID3D11InputLayout> OverlayLayout{};

        if (!Compile(LineShader, "VertexMain", "vs_5_0", VertexCode) || !Compile(LineShader, "PixelMain", "ps_5_0", PixelCode)) {
            return false;
        }

        const std::array<D3D11_INPUT_ELEMENT_DESC, 3> LineElements{{{"START", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Renderer::FLineVertex, mStartAndWidth), D3D11_INPUT_PER_INSTANCE_DATA, 1}, {"END", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Renderer::FLineVertex, mEnd), D3D11_INPUT_PER_INSTANCE_DATA, 1}, {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Renderer::FLineVertex, mColor), D3D11_INPUT_PER_INSTANCE_DATA, 1}}};

        if (FAILED(mDevice->CreateVertexShader(VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), nullptr, LineVertex.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetByteCode().data(), PixelCode->GetByteCode().size(), nullptr, LinePixel.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(LineElements.data(), static_cast<UINT>(LineElements.size()), VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), LineLayout.GetAddressOf()))) {
            Core::ErrorHandler::Report("Renderer", "Line pipeline allocation failed", Core::ErrorHandler::EErrorLevel::Error);
            return false;
        }

        VertexCode.reset();
        PixelCode.reset();

        if (!Compile(GizmoShader, "VertexMain", "vs_5_0", VertexCode) || !Compile(GizmoShader, "PixelMain", "ps_5_0", PixelCode)) {
            return false;
        }

        const std::array<D3D11_INPUT_ELEMENT_DESC, 1> OverlayElements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}}};

        if (FAILED(mDevice->CreateVertexShader(VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), nullptr, GizmoVertex.GetAddressOf())) || FAILED(mDevice->CreatePixelShader(PixelCode->GetByteCode().data(), PixelCode->GetByteCode().size(), nullptr, GizmoPixel.GetAddressOf())) || FAILED(mDevice->CreateInputLayout(OverlayElements.data(), static_cast<UINT>(OverlayElements.size()), VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), OverlayLayout.GetAddressOf()))) {
            Core::ErrorHandler::Report("Renderer", "Gizmo pipeline allocation failed", Core::ErrorHandler::EErrorLevel::Error);
            return false;
        }

        if (!Compile("Text.hlsl", "VertexMain", "vs_5_0", VertexCode) || !Compile("Text.hlsl", "PixelMain", "ps_5_0", PixelCode) || !mTextRenderer->CreateShaders(mDevice.Get(), VertexCode->GetByteCode(), PixelCode->GetByteCode())) {
            return false;
        }

        if (!Compile("Sprite.hlsl", "VertexMain", "vs_5_0", VertexCode) || !Compile("Sprite.hlsl", "PixelMain", "ps_5_0", PixelCode) || !mSpriteRenderer->CreateShaders(mDevice.Get(), VertexCode->GetByteCode(), PixelCode->GetByteCode())) {
            return false;
        }

        if (!Compile("PostProcess.hlsl", "VertexMain", "vs_5_0", VertexCode) || !Compile("PostProcess.hlsl", "PixelMain", "ps_5_0", PixelCode) || !mPostProcessResources->CreateShaders(mDevice.Get(), VertexCode->GetByteCode(), PixelCode->GetByteCode())) {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11VertexShader> DepthClearVertex{};

        if (!Compile("Depth.hlsl", "ClearMain", "vs_5_0", VertexCode) || FAILED(mDevice->CreateVertexShader(VertexCode->GetByteCode().data(), VertexCode->GetByteCode().size(), nullptr, DepthClearVertex.GetAddressOf()))) {
            Core::ErrorHandler::Report("Renderer", "Depth clear shader allocation failed", Core::ErrorHandler::EErrorLevel::Error);
            return false;
        }

        mDepthClearVertexShader = std::move(DepthClearVertex);
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
    bool Renderer::FRenderer::CreateBackBuffer(Core::Uint32 Width, Core::Uint32 Height) {
        mContext->OMSetRenderTargets(0, nullptr, nullptr);
        mBackBuffer.reset();

        const UINT Flags{mTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u};

        if (FAILED(mSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, Flags))) {
            Core::ErrorHandler::Fatal("Renderer", "Swap chain resize failed");
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture{};
        auto BackBuffer{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (FAILED(mSwapChain->GetBuffer(0, IID_PPV_ARGS(Texture.GetAddressOf()))) || !BackBuffer->AttachResource(Texture.Get(), Asset::ETextureUsage::RenderTarget)) {
            Core::ErrorHandler::Fatal("Renderer", "Back buffer allocation failed");
        }

        mBackBuffer = std::move(BackBuffer);
        mWidth = Width;
        mHeight = Height;

        return true;
    }

    /**
     * @brief 메시·선분·기즈모 셰이더, 입력 레이아웃과 고정 파이프라인 상태를 생성합니다.
     */
    bool Renderer::FRenderer::CreatePipeline() {
        mLightingResources = std::make_unique<Renderer::FLightingResources>();
        mShadowResources = std::make_unique<Renderer::FShadowResources>();
        mShadowDepthPass = std::make_unique<Renderer::FShadowDepthPass>(*this, mDevice.Get(), mContext.Get(), mMeshes);
        mBasePass = std::make_unique<Renderer::FBasePass>(*this, mDevice.Get(), mContext.Get(), mMeshes);
        mTextRenderer = std::make_unique<Renderer::FTextRenderer>();
        mSpriteRenderer = std::make_unique<Renderer::FSpriteRenderer>();
        mPostProcessResources = std::make_unique<Renderer::FPostProcessResources>();

        if (!mPostProcessResources->Initialize(mDevice.Get())) {
            return false;
        }

        if (!mSpriteRenderer->Initialize(mDevice.Get())) {
            return false;
        }

        if (!mTextRenderer->Initialize(mDevice.Get(), FindTextContent(mShaderDirectory.parent_path(), "Fonts/Default.json"))) {
            return false;
        }

        if (!CreateShaders()) {
            return false;
        }

        mDefaultMaterial = CoreUObject::NewObject<Asset::UMaterial>();
        mDefaultMaterial->SetShaderPath(Core::FString{"Shaders/"} + MeshShader);
        mDefaultMaterialTexture = CoreUObject::NewObject<Asset::UTexture2D>();

        if (!mDefaultMaterialTexture->SetPixels(1, 1, Asset::ETextureFormat::RGBA8, Core::TArray<Core::Uint8>{255, 255, 255, 255}) || !mDefaultMaterialTexture->CreateResources(mDevice.Get()) || GetMaterialPipeline(*mDefaultMaterial) == nullptr) {
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
        Buffer.ByteWidth = sizeof(Core::FMatrix);
        Buffer.Usage = D3D11_USAGE_DEFAULT;
        Buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        if (FAILED(mDevice->CreateBuffer(&Buffer, nullptr, mViewBuffer.GetAddressOf()))) {
            Core::ErrorHandler::Fatal("Renderer", "View buffer allocation failed");
        }

        Buffer.ByteWidth = sizeof(Core::FVector4);

        if (FAILED(mDevice->CreateBuffer(&Buffer, nullptr, mLineViewBuffer.GetAddressOf()))) {
            return false;
        }

        Buffer.ByteWidth = sizeof(Renderer::FEditorMeshConstants);

        if (FAILED(mDevice->CreateBuffer(&Buffer, nullptr, mEditorMeshBuffer.GetAddressOf()))) {
            Core::ErrorHandler::Fatal("Renderer", "Editor mesh constant buffer allocation failed");
        }

        D3D11_RASTERIZER_DESC Rasterizer{};
        Rasterizer.FillMode = D3D11_FILL_SOLID;
        Rasterizer.CullMode = D3D11_CULL_NONE;
        Rasterizer.DepthClipEnable = true;
        Rasterizer.AntialiasedLineEnable = true;

        if (FAILED(mDevice->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf()))) {
            Core::ErrorHandler::Fatal("Renderer", "Rasterizer allocation failed");
        }

        Rasterizer.FillMode = D3D11_FILL_WIREFRAME;

        if (FAILED(mDevice->CreateRasterizerState(&Rasterizer, mWireframeRasterizer.GetAddressOf()))) {
            return false;
        }

        Rasterizer.FillMode = D3D11_FILL_SOLID;
        Rasterizer.AntialiasedLineEnable = false;
        Rasterizer.ScissorEnable = true;

        if (FAILED(mDevice->CreateRasterizerState(&Rasterizer, mScissorRasterizer.GetAddressOf()))) {
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC Depth{};
        Depth.DepthEnable = true;
        Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        Depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        if (FAILED(mDevice->CreateDepthStencilState(&Depth, mDepthEnabled.GetAddressOf()))) {
            Core::ErrorHandler::Fatal("Renderer", "Depth state allocation failed");
        }

        Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        if (FAILED(mDevice->CreateDepthStencilState(&Depth, mDepthReadOnly.GetAddressOf()))) {
            Core::ErrorHandler::Fatal("Renderer", "Depth state allocation failed");
        }

        Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        Depth.DepthFunc = D3D11_COMPARISON_ALWAYS;

        if (FAILED(mDevice->CreateDepthStencilState(&Depth, mDepthAlways.GetAddressOf()))) {
            Core::ErrorHandler::Fatal("Renderer", "Depth state allocation failed");
        }

        Depth.DepthEnable = false;
        Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        Depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        Core::ErrorHandler::ReportHRESULT(mDevice->CreateDepthStencilState(&Depth, mDepthDisabled.GetAddressOf()), "Renderer", "Depth state allocation failed", Core::ErrorHandler::EErrorLevel::Critical);

        return true;
    }

    /**
     * @brief 월드 선은 깊이를 기록하고 편집 오버레이 선은 깊이 검사 없이 그립니다.
     */
    void Renderer::FRenderer::DrawLines(const Engine::FPrimitiveDrawInterface* Elements, bool DepthTest) {
        mContext->RSSetState(mRasterizer.Get());
        mLineVertices.clear();

        for (const Engine::FDrawLine& Line : Elements->GetLines()) {
            if (Line.mDepthTest == DepthTest) {
                mLineVertices.emplace_back(Line);
            }
        }

        if (mLineVertices.empty()) {
            return;
        }

        const Core::Uint32 Count{static_cast<Core::Uint32>(mLineVertices.size())};

        Core::FString Error{};

        if (!mLineBuffer.Upload(mContext.Get(), std::as_bytes(std::span{mLineVertices}), sizeof(Renderer::FLineVertex), Count + 256, Error)) {
            Core::ErrorHandler::Fatal("Renderer", Error);
            return;
        }

        const UINT Stride{sizeof(Renderer::FLineVertex)};
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
        Renderer::RendererStats::Triangles.Add(static_cast<double>(Count) * 2.0);
        mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);

        ++mStats.mDrawCalls;
    }

    /**
     * @brief 고정 에디터 메시의 로컬 정점과 인덱스를 종류별 불변 GPU 버퍼에 업로드합니다.
     */
    bool Renderer::FRenderer::CreateEditorMeshes() {
        for (std::size_t Index{}; Index < mEditorMeshes.size(); ++Index) {
            auto Resource{CoreUObject::NewObject<Asset::UStaticMesh>()};

            if (!Resource->SetMeshData(Engine::GetEditorMesh(static_cast<Engine::EEditorMesh>(Index))) || !Resource->CreateResources(mDevice.Get())) {
                return false;
            }

            mEditorMeshes[Index] = std::move(Resource);
        }

        return true;
    }

    void Renderer::FRenderer::DrawMeshes(const Engine::FPrimitiveDrawInterface* Elements, bool DepthTest) {
        mContext->RSSetState(mRasterizer.Get());

        const UINT Stride{Asset::UStaticMesh::GetPositionStride()};
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

        for (const Engine::FDrawMesh& Draw : Elements->GetMeshes()) {
            if (Draw.mMesh >= Engine::EEditorMesh::Count || Draw.mDepthTest != DepthTest) {
                continue;
            }

            const Asset::UStaticMesh* Mesh{mEditorMeshes[static_cast<std::size_t>(Draw.mMesh)].get()};

            if (Mesh == nullptr || !Mesh->HasResources()) {
                continue;
            }

            const Renderer::FEditorMeshConstants Constants{Draw.mWorld, {Draw.mColor.x, Draw.mColor.y, Draw.mColor.z, Draw.mColor.w}};
            mContext->UpdateSubresource(mEditorMeshBuffer.Get(), 0, nullptr, &Constants, 0, 0);

            ID3D11Buffer* Buffer{Mesh->GetPositionBuffer()};
            mContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
            mContext->IASetIndexBuffer(Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
            mContext->DrawIndexed(Mesh->GetIndexCount(), 0, 0);
            Renderer::RendererStats::Triangles.Add(Mesh->GetIndexCount() / 3);
            ++mStats.mDrawCalls;
        }

        mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
    }

    void Renderer::FRenderer::DrawOverlayTexts(const Engine::FSceneViewFamily& Family) {
        mContext->RSSetState(mRasterizer.Get());

        if (mTextRenderer == nullptr || !Family.mShowFlags.mText) {
            return;
        }

        Core::TArray<Engine::FDrawText> Texts{};

        if (Family.mEditorElements != nullptr) {
            for (const Engine::FDrawText& Text : Family.mEditorElements->GetTexts()) {
                if (!Text.mDepthTest) {
                    Texts.push_back(Text);
                }
            }
        }

        std::stable_sort(Texts.begin(), Texts.end(), [&Family](const Engine::FDrawText& First, const Engine::FDrawText& Second) {
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

        for (const Engine::FDrawText& Text : Texts) {
            if (mTextRenderer->Draw(mContext.Get(), Text, Family.mView)) {
                ++mStats.mDrawCalls;
            }
        }

        mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
    }

    Renderer::FMaterialPipeline* Renderer::FRenderer::GetMaterialPipeline(const Asset::UMaterial& Material, Engine::EViewMode Mode) {
        const Core::FString& Relative{Material.GetShaderPath()};
        const std::filesystem::path RelativePath{Core::FilePath::FromUtf8(Relative).lexically_normal()};
        Core::FString Key{Core::FilePath::ToGenericUtf8(RelativePath) + (Material.IsTwoSided() ? "|TwoSided" : "|OneSided")};
        std::transform(Key.begin(), Key.end(), Key.begin(), [](unsigned char Character) {
            return static_cast<char>(std::tolower(Character));
        });

        Key += "|" + std::to_string(static_cast<Core::Uint32>(Mode)) + (Material.GetNormalMapTexture() != nullptr ? "|NormalMap" : "|VertexNormal");

        const auto Existing{mMaterialPipelines.find(Key)};

        if (Existing != mMaterialPipelines.end()) {
            return Existing->second.get();
        }

        if (mShadersDirty) {
            return nullptr;
        }

        const bool Builtin{RelativePath == "Shaders/Lit.hlsl" || RelativePath == "Shaders/Unlit.hlsl" || RelativePath == "Shaders/Mesh.hlsl" || Mode == Engine::EViewMode::Unlit || Mode == Engine::EViewMode::WorldNormal};
        const std::filesystem::path Source{FindTextContent(mShaderDirectory.parent_path(), Mode == Engine::EViewMode::Unlit || Mode == Engine::EViewMode::WorldNormal ? std::filesystem::path{"Shaders/Lit.hlsl"} : RelativePath, Builtin)};
        RenderCore::FShaderDescription Description{};
        Description.mSourcePath = Source;
        Description.mIncludeDirectories = {Source.parent_path(), mShaderDirectory};

        const char* Model{Mode == Engine::EViewMode::Lit_Gouraud ? "LIGHTING_MODEL_GOURAUD" : Mode == Engine::EViewMode::Lit_Lambert ? "LIGHTING_MODEL_LAMBERT" : "LIGHTING_MODEL_PHONG"};
        Description.mMacros.push_back({Model, "1"});

        if (Builtin) {
            Description.mMacros.push_back({"MacawInstanced", "1"});
        }

        if (Mode == Engine::EViewMode::Unlit) {
            Description.mMacros.push_back({"MACAW_UNLIT", "1"});
        }

        if (Mode == Engine::EViewMode::WorldNormal) {
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

        Core::FString Diagnostics{};
        std::shared_ptr<const RenderCore::FShader> Vertex{mShaderManager.Load(Description, Diagnostics)};
        Core::FString Error{Diagnostics};

        Description.mEntryPoint = "PixelMain";
        Description.mTargetProfile = "ps_5_0";

        std::shared_ptr<const RenderCore::FShader> Pixel{mShaderManager.Load(Description, Diagnostics)};

        Error += Diagnostics;

        std::unique_ptr<Renderer::FMaterialPipeline> Pipeline{};

        if (Vertex != nullptr && Pixel != nullptr) {
            Pipeline = std::make_unique<Renderer::FMaterialPipeline>(Renderer::EMeshPass::BasePass, std::move(Vertex), std::move(Pixel), Material.IsTwoSided(), Builtin);

            if (!Pipeline->CreateResources(mDevice.Get(), Diagnostics)) {
                Error += Diagnostics;
                Pipeline.reset();
            }
        }

        if (!Error.empty()) {
            Core::ErrorHandler::Report("Material", Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        Renderer::FMaterialPipeline* Result{Pipeline.get()};

        mMaterialPipelines.emplace(std::move(Key), std::move(Pipeline));

        return Result;
    }

    Asset::UMaterial* Renderer::FRenderer::GetDefaultMaterial() const {
        return mDefaultMaterial.get();
    }

    Renderer::FMaterialPipeline* Renderer::FRenderer::GetDepthPipeline() {
        const Core::FString Key{"shaders/depth.hlsl|depth"};
        const auto Existing{mMaterialPipelines.find(Key)};

        if (Existing != mMaterialPipelines.end()) {
            return Existing->second.get();
        }

        if (mShadersDirty) {
            return nullptr;
        }

        RenderCore::FShaderDescription Description{};
        Description.mSourcePath = FindTextContent(mShaderDirectory.parent_path(), "Shaders/Depth.hlsl", true);
        Description.mIncludeDirectories = {Description.mSourcePath.parent_path(), mShaderDirectory};
        Description.mMacros.push_back({"MacawInstanced", "1"});
        Description.mEntryPoint = "VertexMain";
        Description.mTargetProfile = "vs_5_0";

#ifdef _DEBUG
        Description.mCompileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        Core::FString Diagnostics{};
        std::shared_ptr<const RenderCore::FShader> Vertex{mShaderManager.Load(Description, Diagnostics)};
        Core::FString Error{Diagnostics};
        std::unique_ptr<Renderer::FMaterialPipeline> Pipeline{};

        if (Vertex != nullptr) {
            Pipeline = std::make_unique<Renderer::FMaterialPipeline>(Renderer::EMeshPass::ShadowDepth, std::move(Vertex), nullptr, true, true);

            if (!Pipeline->CreateResources(mDevice.Get(), Diagnostics)) {
                Error += Diagnostics;
                Pipeline.reset();
            }
        }

        if (!Error.empty()) {
            Core::ErrorHandler::Report("Shadow", Error, Pipeline != nullptr ? Core::ErrorHandler::EErrorLevel::Warning : Core::ErrorHandler::EErrorLevel::Error);
        }

        Renderer::FMaterialPipeline* Result{Pipeline.get()};

        mMaterialPipelines.emplace(Key, std::move(Pipeline));

        return Result;
    }

    void Renderer::FRenderer::DrawTranslucency(const Engine::FSceneViewFamily& Family, bool EditorIcons) {
        using FPrimitive = std::variant<const Engine::FPrimitiveSceneProxy*, Engine::FDrawText>;
        using FCommand = std::pair<float, FPrimitive>;
        Core::TArray<FCommand> Commands{};

        const auto Depth{[&Family](const Core::FVector& Position, bool ScreenSpace) {
            const Core::FVector4 Clip{Core::FVector4{Position, 1.0f} * Family.mView.mViewProjection};
            const float Value{ScreenSpace ? Position.z : Clip.w > 0.0f ? Clip.z / Clip.w : -1.0f};
            return std::isfinite(Value) ? Value : -1.0f;
        }};

        Core::TArray<Engine::FPrimitiveSceneProxy> LightIcons{};

        if (EditorIcons && Family.mEditorElements != nullptr && (Family.mShowFlags.mLightIcons || Family.mShowFlags.mFogIcons)) {
            const auto* Scene{static_cast<const Renderer::FScene*>(Family.mScene)};
            LightIcons.reserve(Scene->GetLights().size() + Scene->GetHeightFogs().size());

            for (const Engine::FLightSceneProxy& Light : Scene->GetLights()) {
                if (Family.mShowFlags.mLightIcons) {
                    LightIcons.push_back(Light.CreateIconProxy());
                }
            }

            for (const Engine::FHeightFogSceneProxy& Fog : Scene->GetHeightFogs()) {
                if (Family.mShowFlags.mFogIcons) {
                    LightIcons.push_back(Fog.CreateIconProxy());
                }
            }

            for (const Engine::FPrimitiveSceneProxy& Icon : LightIcons) {
                Commands.emplace_back(Depth(Icon.mWorld.TransformPosition({}), false), FPrimitive{&Icon});
            }
        }

        if (!EditorIcons && Family.mShowFlags.mPrimitives) {
            const auto* Scene{static_cast<const Renderer::FScene*>(Family.mScene)};

            for (const Renderer::FScenePrimitive& Primitive : Scene->GetPrimitives()) {
                const Engine::FPrimitiveSceneProxy& Proxy{Primitive.mProxy};

                if (Proxy.mSprite.has_value() && Family.mShowFlags.mSprites) {
                    Commands.emplace_back(Depth(Proxy.mWorld.TransformPosition({}), false), FPrimitive{&Proxy});
                } else if (Proxy.mText.has_value() && Family.mShowFlags.mText) {
                    Engine::FDrawText Text{*Proxy.mText};
                    Text.mPosition = Proxy.mWorld.TransformPosition(Text.mPosition);
                    Text.mScale.x *= Proxy.mWorld.TransformVector({0.0f, 1.0f, 0.0f}).Length();
                    Text.mScale.y *= Proxy.mWorld.TransformVector({0.0f, 0.0f, 1.0f}).Length();

                    const float Value{Depth(Text.mPosition, Text.mScreenSpace)};
                    Commands.emplace_back(Value, FPrimitive{std::move(Text)});
                }
            }
        }

        if (EditorIcons && Family.mEditorElements != nullptr && Family.mShowFlags.mText) {
            for (const Engine::FDrawText& Text : Family.mEditorElements->GetTexts()) {
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

            if (const auto* Sprite{std::get_if<const Engine::FPrimitiveSceneProxy*>(&Command.second)}) {
                mContext->RSSetState(Family.mViewMode == Engine::EViewMode::Wireframe ? mWireframeRasterizer.Get() : mRasterizer.Get());
                Drawn = mSpriteRenderer != nullptr && mSpriteRenderer->Draw(mContext.Get(), **Sprite, Family.mView);
            } else {
                mContext->RSSetState(mRasterizer.Get());
                Drawn = mTextRenderer != nullptr && mTextRenderer->Draw(mContext.Get(), std::get<Engine::FDrawText>(Command.second), Family.mView);
            }

            if (Drawn) {
                ++mStats.mDrawCalls;
                ++mStats.mVisiblePrimitives;
            }
        }

        mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
    }

    bool Renderer::FRenderer::PrepareLighting(const Engine::FSceneViewFamily& Family) {
        const auto* Scene{static_cast<const Renderer::FScene*>(Family.mScene)};

        if (mShadowResources != nullptr) {
            mShadowResources->Bind(mContext.Get());
        }

        return mLightingResources != nullptr && mLightingResources->Upload(mDevice.Get(), mContext.Get(), Scene->GetLights(), Family.mView, mShadowResources != nullptr ? std::span<const std::array<Core::Uint32, 2>>{mShadowResources->GetLightRanges()} : std::span<const std::array<Core::Uint32, 2>>{});
    }

    void Renderer::FRenderer::FinishLighting() {
        if (mShadowResources != nullptr) {
            mShadowResources->Unbind(mContext.Get());
        }

        if (mLightingResources != nullptr) {
            mLightingResources->Unbind(mContext.Get());
        }
    }

    void Renderer::FRenderer::ClearTarget(const Renderer::FPassTarget& Target, const float* Color) {
        if (Target.mScissor.has_value()) {
            if (Target.mDepth == nullptr) {
                return;
            }

            mContext->OMSetRenderTargets(0, nullptr, Target.mDepth);
            mContext->RSSetViewports(1, &Target.mViewport);
            mContext->RSSetScissorRects(1, &*Target.mScissor);
            mContext->RSSetState(mScissorRasterizer.Get());
            mContext->OMSetDepthStencilState(mDepthAlways.Get(), 0);
            mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
            mContext->IASetInputLayout(nullptr);
            mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mContext->VSSetShader(mDepthClearVertexShader.Get(), nullptr, 0);
            mContext->PSSetShader(nullptr, nullptr, 0);
            mContext->Draw(3, 0);

            return;
        }

        if (Color != nullptr && Target.mColor != nullptr) {
            mContext->ClearRenderTargetView(Target.mColor, Color);
        }

        if (Target.mDepth != nullptr) {
            mContext->ClearDepthStencilView(Target.mDepth, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        }
    }

    void Renderer::FRenderer::DrawShadowDepths(const Renderer::FScene& Scene, const Engine::FSceneViewFamily& Family) {
        const std::span<const Renderer::FGpuShadow> Maps{mShadowResources->GetMaps()};
        const Renderer::FMaterialPipeline* DepthPipeline{Maps.empty() ? nullptr : GetDepthPipeline()};
        const Core::Uint64 DepthShaderRevision{DepthPipeline != nullptr ? DepthPipeline->GetVertexRevision() : 0};

        for (Core::Uint32 Index{}; Index < Maps.size(); ++Index) {
            const Renderer::FShadowTileStamp Stamp{mShadowResources->MakeTileStamp(Index, DepthShaderRevision)};

            if (mShadowResources->IsTileCurrent(Index, Stamp)) {
                continue;
            }

            ClearTarget(mShadowResources->GetClearTarget(Index), nullptr);

            Renderer::FMeshPassParameters ShadowParameters{};
            ShadowParameters.mTarget = mShadowResources->GetTarget(Index);
            ShadowParameters.mViewProjection = Maps[Index].mViewProjection;

            if (DepthPipeline != nullptr && DrawMeshPass(*mShadowDepthPass, Scene, Family, ShadowParameters)) {
                mShadowResources->CommitTile(Index, Stamp);
            }
        }
    }

    bool Renderer::FRenderer::DrawMeshPass(Renderer::FMeshPass& Pass, const Renderer::FScene& Scene, const Engine::FSceneViewFamily& Family, const Renderer::FMeshPassParameters& Parameters) {
        static Core::FStat OpaquePassTime{"render", "Opaque mesh pass CPU (inclusive)", Core::EStatUnit::Milliseconds};
        static Core::FStat ShadowPassTime{"shadow", "Shadow depth pass CPU (inclusive)", Core::EStatUnit::Milliseconds};
        const bool BasePass{Pass.GetType() == Renderer::EMeshPass::BasePass};
        const Core::FScopedStatTimer StatTimer{BasePass ? OpaquePassTime : ShadowPassTime};

        const Renderer::FPassTarget& Target{Parameters.mTarget};
        ID3D11RenderTargetView* Color{Target.mColor};
        mContext->OMSetRenderTargets(Color != nullptr ? 1u : 0u, Color != nullptr ? &Color : nullptr, Target.mDepth);
        mContext->RSSetViewports(1, &Target.mViewport);

        if (Target.mScissor.has_value()) {
            mContext->RSSetScissorRects(1, &*Target.mScissor);
        }

        mContext->RSSetState(mRasterizer.Get());
        mContext->OMSetDepthStencilState(mDepthEnabled.Get(), 0);
        mContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        mContext->UpdateSubresource(mViewBuffer.Get(), 0, nullptr, &Parameters.mViewProjection, 0, 0);
        ID3D11Buffer* ViewBuffer{mViewBuffer.Get()};
        mContext->VSSetConstantBuffers(1, 1, &ViewBuffer);
        mContext->PSSetConstantBuffers(1, 1, &ViewBuffer);

        if (BasePass && !PrepareLighting(Family)) {
            FinishLighting();
            return false;
        }

        Core::TArray<Renderer::FMeshDrawCommand> Commands{};
        Core::Uint32 Primitives{};
        const bool Built{Pass.BuildCommands(Scene, Family, Parameters.mViewProjection, Commands, Primitives)};

        if (Built) {
            const Core::Uint32 DrawCalls{SubmitMeshCommands(Family, Pass.GetType(), Commands)};
            (BasePass ? mStats.mDrawCalls : mStats.mShadowDrawCalls) += DrawCalls;
            mStats.mVisiblePrimitives += BasePass ? Primitives : 0u;
        }

        if (BasePass) {
            FinishLighting();
            mStats.mTransformUpdates += Scene.TakeTransformUpdates();
        }

        return Built;
    }

    Core::Uint32 Renderer::FRenderer::SubmitMeshCommands(const Engine::FSceneViewFamily& Family, Renderer::EMeshPass Pass, std::span<const Renderer::FMeshDrawCommand> Commands) {
        const bool BasePass{Pass == Renderer::EMeshPass::BasePass};

        if (std::any_of(Commands.begin(), Commands.end(), [](const Renderer::FMeshDrawCommand& Command) {
            return Command.mPipeline->IsInstanced();
        })) {
            constexpr Core::Uint32 MaximumCapacity{std::numeric_limits<Core::Uint32>::max() / sizeof(Renderer::FObjectConstants)};

            if (Commands.size() > MaximumCapacity) {
                Core::ErrorHandler::Fatal("Renderer", "Mesh instance buffer capacity exceeded");
            }

            Core::TArray<Renderer::FObjectConstants> Instances{};
            Instances.reserve(Commands.size());

            for (const Renderer::FMeshDrawCommand& Command : Commands) {
                Instances.push_back(Command.mPrimitive->mObjectConstants);
            }

            const Core::Uint32 Count{static_cast<Core::Uint32>(Instances.size())};
            const Core::Uint32 Capacity{std::min(std::bit_ceil(Count), MaximumCapacity)};
            Core::FString Error{};

            if (!mMeshInstanceBuffer.Upload(mContext.Get(), std::as_bytes(std::span<const Renderer::FObjectConstants>{Instances}), sizeof(Renderer::FObjectConstants), Capacity, Error)) {
                Core::ErrorHandler::Fatal("Renderer", Error);
            }

            ID3D11Buffer* InstanceBuffer{mMeshInstanceBuffer.GetBuffer()};
            const UINT Stride{sizeof(Renderer::FObjectConstants)};
            const UINT Offset{};
            mContext->IASetVertexBuffers(4, 1, &InstanceBuffer, &Stride, &Offset);
        }

        Renderer::FMaterialPipeline* LastPipeline{};
        Asset::UMaterial* LastMaterial{};
        Asset::UStaticMesh* LastMesh{};
        Core::Uint32 DrawCalls{};
        const std::array<UINT, 4> Offsets{};

        for (std::size_t First{}; First < Commands.size();) {
            const Renderer::FMeshDrawCommand& Command{Commands[First]};
            std::size_t End{First + 1};

            if (Command.mPipeline->IsInstanced()) {
                while (End < Commands.size() && Commands[End].mPipeline == Command.mPipeline && Commands[End].mMaterial == Command.mMaterial && Commands[End].mMesh == Command.mMesh && Commands[End].mFirstIndex == Command.mFirstIndex && Commands[End].mIndexCount == Command.mIndexCount) {
                    ++End;
                }
            }

            const Core::Uint32 InstanceCount{static_cast<Core::Uint32>(End - First)};

            if (LastPipeline != Command.mPipeline) {
                if (BasePass) {
                    Renderer::RendererStats::PipelineBinds.Add();
                }

                Command.mPipeline->Bind(mContext.Get());

                if (BasePass && Family.mViewMode == Engine::EViewMode::Wireframe) {
                    mContext->RSSetState(mWireframeRasterizer.Get());
                }

                LastPipeline = Command.mPipeline;
                LastMaterial = nullptr;
            }

            if (Command.mMaterial != nullptr && LastMaterial != Command.mMaterial) {
                Renderer::RendererStats::MaterialBinds.Add();
                ID3D11Buffer* Constants{Command.mMaterial->GetConstantBuffer()};
                Asset::UTexture2D* Texture{Command.mMaterial->GetBaseColorTexture()};
                ID3D11ShaderResourceView* Resource{Texture != nullptr ? Texture->GetShaderResourceView() : mDefaultMaterialTexture->GetShaderResourceView()};
                ID3D11SamplerState* Sampler{mMaterialSampler.Get()};
                mContext->VSSetConstantBuffers(2, 1, &Constants);
                mContext->PSSetConstantBuffers(2, 1, &Constants);
                Asset::UTexture2D* NormalTexture{Command.mMaterial->GetNormalMapTexture()};
                ID3D11ShaderResourceView* NormalResource{NormalTexture != nullptr ? NormalTexture->GetShaderResourceView() : nullptr};
                mContext->PSSetShaderResources(0, 1, &Resource);
                mContext->VSSetShaderResources(1, 1, &NormalResource);
                mContext->PSSetShaderResources(1, 1, &NormalResource);
                mContext->VSSetSamplers(0, 1, &Sampler);
                mContext->PSSetSamplers(0, 1, &Sampler);
                LastMaterial = Command.mMaterial;
            }

            if (LastMesh != Command.mMesh) {
                if (BasePass) {
                    Renderer::RendererStats::MeshBinds.Add();
                }

                Asset::UStaticMesh* Mesh{Command.mMesh};
                const std::array<ID3D11Buffer*, 4> Buffers{Mesh->GetPositionBuffer(), Mesh->GetTangentBuffer(), Mesh->GetUVBuffer(), Mesh->GetColorBuffer()};
                const std::array<UINT, 4> Strides{Asset::UStaticMesh::GetPositionStride(), Asset::UStaticMesh::GetTangentStride(), Mesh->GetUVStride(), Asset::UStaticMesh::GetColorStride()};
                mContext->IASetVertexBuffers(0, static_cast<UINT>(Buffers.size()), Buffers.data(), Strides.data(), Offsets.data());
                mContext->IASetIndexBuffer(Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
                LastMesh = Mesh;
            }

            if (Command.mPipeline->IsInstanced()) {
                mContext->DrawIndexedInstanced(Command.mIndexCount, InstanceCount, Command.mFirstIndex, 0, static_cast<UINT>(First));
            } else {
                ID3D11Buffer* ObjectBuffer{Command.mPrimitive->mObjectBuffer.Get()};
                mContext->VSSetConstantBuffers(0, 1, &ObjectBuffer);
                mContext->PSSetConstantBuffers(0, 1, &ObjectBuffer);
                mContext->DrawIndexed(Command.mIndexCount, Command.mFirstIndex, 0);
            }

            if (BasePass) {
                Renderer::RendererStats::Triangles.Add(static_cast<double>(Command.mIndexCount / 3) * InstanceCount);
            }

            ++DrawCalls;
            First = End;
        }

        ID3D11Buffer* EmptyInstanceBuffer{};
        mContext->IASetVertexBuffers(4, 1, &EmptyInstanceBuffer, Offsets.data(), Offsets.data());

        if (BasePass) {
            ID3D11ShaderResourceView* Empty{};
            mContext->PSSetShaderResources(0, 1, &Empty);
            mContext->VSSetShaderResources(1, 1, &Empty);
            mContext->PSSetShaderResources(1, 1, &Empty);
        }

        return DrawCalls;
    }

}
