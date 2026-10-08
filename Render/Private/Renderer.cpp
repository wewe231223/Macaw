#include "pch.h"
#include "Render/Renderer.h"
#include "FRendererScene.h"
#include "RenderCore/FSceneUpdateBatch.h"
#include "Asset/UTexture.h"
#include "Asset/UMesh.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Stat/Stat.h"

#include <ranges>
#include <utility>
#include <atomic>
#include <dxgi1_6.h>
#include "Core/Console/Console.h"

namespace {
    std::atomic<Uint64> NextSceneGeneration{1};

    Uint64 AllocateSceneGeneration() {
        return NextSceneGeneration.fetch_add(1, std::memory_order_relaxed);
    }
}

FRenderer::~FRenderer() {
    ReleaseScenes();

#if EnableFrameResourceFence
    if (mFrameFenceEvent != nullptr) {
        CloseHandle(mFrameFenceEvent);
    }
#endif
}

void FRenderer::Create(HWND WindowHandle, UINT Width, UINT Height) {
    mBackBufferWidth = Width;
    mBackBufferHeight = Height;

    FRenderer::CreateDeviceAndSwapChain(WindowHandle);

    auto BackBuffer{std::make_unique<FSceneRenderSurface>()};

    BackBuffer->InitializeSwapChain(mDevice.Get(), mSwapChain.Get());
    mBackBufferSurface = std::move(BackBuffer);

#ifdef _DEBUG
    mDevice.As(&mDebugInterface);
#endif
}

bool FRenderer::Initialize() {
    if (!CreateSamplerStates() || !mSceneRenderer.Initialize(mDevice.Get()) || !mOverLayRenderer.Initialize(mDevice.Get(), mDeviceContext.Get(), mFrameResourceCount)) {
        return false;
    }

#if EnableFrameResourceFence
    Microsoft::WRL::ComPtr<ID3D11Device5> FenceDevice{};

    if (FAILED(mDevice.As(&FenceDevice)) || FAILED(mDeviceContext.As(&mFenceContext)) || FAILED(FenceDevice->CreateFence(0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(mFrameFence.GetAddressOf())))) {
        return false;
    }

    mFrameFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (mFrameFenceEvent == nullptr) {
        return false;
    }
#endif

    for (FFrameResource& FrameResource : mFrameResources) {
        if (!FrameResource.Initialize(mDevice.Get(), mDeviceContext.Get())) {
            return false;
        }
    }


    return true;
}

void FRenderer::BeginUiRender() {
    mBackBufferSurface->Bind(mDeviceContext.Get());
    mBackBufferSurface->Clear(mDeviceContext.Get(), mUiClearColor);
}

void FRenderer::BindSamplerStates() {
    std::array<ID3D11SamplerState*, 6> RawSamplerStates{};
    std::ranges::transform(mSamplerStates, RawSamplerStates.begin(), [](const auto& Sampler) {
        return Sampler.Get();
    });

    mDeviceContext->PSSetSamplers(0, static_cast<UINT>(RawSamplerStates.size()), RawSamplerStates.data());
    mDeviceContext->VSSetSamplers(0, static_cast<UINT>(RawSamplerStates.size()), RawSamplerStates.data());
}

void FRenderer::EndFrame() {
    if (mCurrentFrameResource != nullptr) {
#if EnableFrameResourceFence
        const HRESULT Result{mFenceContext->Signal(mFrameFence.Get(), mNextFenceValue)};

        ErrorHandler::ReportHRESULT(Result, "[ FRenderer ]", "Failed to signal the frame fence.", ErrorHandler::EErrorLevel::Critical);
        mCompletionValues[mNextFrameResourceIndex] = mNextFenceValue;
        ++mNextFenceValue;
        mNextFrameResourceIndex = (mNextFrameResourceIndex + 1) % mFrameResourceCount;
#endif

        mOverLayRenderer.EndFrame();
        mCurrentFrameResource->EndFrame();
        mCurrentFrameResource = nullptr;
    }

    mSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

ID3D11Device* FRenderer::GetDevice() const {
    return mDevice.Get();
}

ID3D11DeviceContext* FRenderer::GetDeviceContext() const {
    return mDeviceContext.Get();
}

bool FRenderer::BindAssetRegistry(const IAssetRegistry* InAssetRegistry, IAssetRegistryMutator* InAssetRegistryMutator) {
    if (mAssetRegistry == InAssetRegistry) {
        return mOverLayRenderer.BindAssetRegistry(InAssetRegistry, InAssetRegistryMutator);
    }

    ResetScenes();
    mOverLayRenderer.BindAssetRegistry(nullptr);

    mAssetResources.Reset();
    mAssetRegistry = nullptr;

    if (InAssetRegistry != nullptr && !mAssetResources.Initialize(mDevice.Get())) {
        return false;
    }

    if (!mOverLayRenderer.BindAssetRegistry(InAssetRegistry, InAssetRegistryMutator)) {
        mAssetResources.Reset();
        return false;
    }

    mAssetRegistry = InAssetRegistry;

    return true;
}

void FRenderer::BeginFrame() {
    if (mCurrentFrameResource != nullptr) {
        return;
    }

#if EnableFrameResourceFence
    FFrameResource& FrameResource{mFrameResources[mNextFrameResourceIndex]};
    const Uint64 CompletionValue{mCompletionValues[mNextFrameResourceIndex]};

    if (CompletionValue != 0 && mFrameFence->GetCompletedValue() < CompletionValue) {
        const HRESULT Result{mFrameFence->SetEventOnCompletion(CompletionValue, mFrameFenceEvent)};

        ErrorHandler::ReportHRESULT(Result, "[ FRenderer ]", "Failed to register the frame fence event.", ErrorHandler::EErrorLevel::Critical);

        mDeviceContext->Flush();

        for (;;) {
            const DWORD WaitResult{WaitForSingleObject(mFrameFenceEvent, 1000)};

            if (WaitResult == WAIT_OBJECT_0) {
                break;
            }

            if (WaitResult != WAIT_TIMEOUT || FAILED(mDevice->GetDeviceRemovedReason())) {
                ErrorHandler::Report("[ FRenderer ]", "Failed to wait for the frame fence.", ErrorHandler::EErrorLevel::Critical);
                return;
            }
        }
    }
#else
    FFrameResource& FrameResource{mFrameResources.front()};
#endif

    if (!FrameResource.BeginFrame(mDeviceContext.Get())) {
        ErrorHandler::Report("[ FRenderer ]", "Failed to begin a frame resource.", ErrorHandler::EErrorLevel::Critical);
        return;
    }

    const Uint32 FrameResourceIndex{static_cast<Uint32>(&FrameResource - mFrameResources.data())};

    if (!mOverLayRenderer.BeginFrame(mDeviceContext.Get(), FrameResourceIndex, mFrameSerial + 1)) {
        FrameResource.EndFrame();
        ErrorHandler::Report("[ FRenderer ]", "Failed to begin overlay frame resources.", ErrorHandler::EErrorLevel::Critical);
        return;
    }

    mCurrentFrameResource = &FrameResource;
    ++mFrameSerial;
    mSceneRenderer.BeginFrame(mFrameSerial);

    if (mAssetRegistry != nullptr) {
        mAssetResources.Prune(*mAssetRegistry);
    }
}

const FRenderScene& FRenderer::SynchronizeScene(FSceneRenderData& Scene) {
    const Uint64 SceneId{Scene.mSceneId != 0 ? Scene.mSceneId : mTransientSceneId};
    FRendererScene& RendererScene{FindOrAddScene(SceneId)};
    FSceneUpdateBatch Updates{RendererScene.GetHandle(), {}, false};

    std::swap(Updates.mRenderData, Scene);
    Updates.mRenderData.mSceneId = SceneId;

    const bool Applied{ApplySceneUpdates(Updates)};

    std::swap(Updates.mRenderData, Scene);
    ErrorHandler::Report(!Applied, "FRenderer", "Cannot synchronize render scene", ErrorHandler::EErrorLevel::Error);

    return RendererScene.GetRenderScene();
}

std::weak_ptr<FSceneInterface> FRenderer::CreateScene() {
    const Uint64 SceneId{AllocateRenderSceneId()};

    return mRenderScenes.emplace(SceneId, std::make_shared<FRendererScene>(*this, FSceneHandle{SceneId, AllocateSceneGeneration()})).first->second;
}

bool FRenderer::ReleaseScene(FSceneHandle Handle) {
    const auto Position{mRenderScenes.find(Handle.mId)};

    if (!Handle.IsValid() || Position == mRenderScenes.end() || Position->second->GetHandle() != Handle) {
        return false;
    }

    ReleaseSceneResources(Handle.mId);
    Position->second->Detach();
    mRenderScenes.erase(Position);

    return true;
}

void FRenderer::ResetScenes() {
    mSceneRenderer.ResetScenes();

    for (FFrameResource& FrameResource : mFrameResources) {
        FrameResource.ResetScenes();
    }

    for (const auto& Entry : mRenderScenes) {
        Entry.second->Reset(AllocateSceneGeneration());
    }
}

FSceneHandle FRenderer::GetSceneHandle(Uint64 SceneId) const {
    const auto Position{mRenderScenes.find(SceneId)};

    return Position != mRenderScenes.end() ? Position->second->GetHandle() : FSceneHandle{};
}

const FRenderScene* FRenderer::FindScene(FSceneHandle Handle) const {
    const auto Position{mRenderScenes.find(Handle.mId)};

    return Handle.IsValid() && Position != mRenderScenes.end() && Position->second->GetHandle() == Handle ? &Position->second->GetRenderScene() : nullptr;
}

FRendererScene& FRenderer::FindOrAddScene(Uint64 SceneId) {
    const auto Position{mRenderScenes.find(SceneId)};

    if (Position != mRenderScenes.end()) {
        return *Position->second;
    }

    return *mRenderScenes.emplace(SceneId, std::make_shared<FRendererScene>(*this, FSceneHandle{SceneId, AllocateSceneGeneration()})).first->second;
}

bool FRenderer::ApplySceneUpdates(FSceneUpdateBatch& Updates) {
    const auto Position{mRenderScenes.find(Updates.mSceneHandle.mId)};

    if (!Updates.mSceneHandle.IsValid() || Position == mRenderScenes.end() || Position->second->GetHandle() != Updates.mSceneHandle || Updates.mRenderData.mSceneId != Updates.mSceneHandle.mId) {
        return false;
    }

    const Stat::FScopedRenderPreparationStatTimer StageStat{Stat::ERenderPreparationStage::SceneSynchronization};
    FMaterialBuffer& Materials{mAssetResources.GetMaterialBuffer()};

    if (mAssetRegistry != nullptr && mDeviceContext != nullptr) {
        Materials.Synchronize(*mAssetRegistry, mDeviceContext.Get());
    }

    if (Updates.mFullSnapshot) {
        ReleaseSceneResources(Updates.mSceneHandle.mId);
        Position->second->ResetRenderData();
    }

    Position->second->GetRenderScene().Synchronize(mAssetRegistry, Updates, Materials);

    return true;
}

void FRenderer::ReleaseSceneResources(Uint64 SceneId) {
    mSceneRenderer.ReleaseScene(SceneId);

    for (FFrameResource& FrameResource : mFrameResources) {
        FrameResource.ReleaseScene(SceneId);
    }
}

void FRenderer::ReleaseScenes() {
    mSceneRenderer.ResetScenes();

    for (FFrameResource& FrameResource : mFrameResources) {
        FrameResource.ResetScenes();
    }

    for (const auto& Entry : mRenderScenes) {
        Entry.second->Detach();
    }

    mRenderScenes.clear();
}

void FRenderer::RenderView(const FRenderView& View, FSceneRenderData& Scene, const FOverlayRenderData& Overlay) {
    const FRenderScene& RenderScene{SynchronizeScene(Scene)};

    RenderView(View, RenderScene, Overlay);
}

void FRenderer::RenderView(const FRenderView& View, const FRenderScene& Scene, const FOverlayRenderData& Overlay) {
    const Stat::FScopedSystemStatTimer RenderStat{Stat::ESystemStatStage::RenderPreparation};

    if (mDeviceContext == nullptr || mCurrentFrameResource == nullptr) {
        return;
    }

    BindSamplerStates();

    const FRenderContext Context{mDeviceContext.Get(), mAssetRegistry, &mAssetResources, mAssetResources.GetMaterialBuffer().GetSRV(), mCurrentFrameResource};
    const FSceneRenderOutput Output{mSceneRenderer.RenderView(Context, View, Scene)};

    if (!Output.IsValid()) {
        return;
    }

    const Stat::FScopedSystemStatTimer StageStat{Stat::ESystemStatStage::EditorOverlays};
    BindSamplerStates();
    mOverLayRenderer.RenderView(mDeviceContext.Get(), View, Overlay, Output);
    View.mTarget->Bind(mDeviceContext.Get());
}

void FRenderer::ReSize(Uint32 Width, Uint32 Height) {
    if (!mSwapChain || Width == 0 || Height == 0) {
        return;
    }

    mDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    mBackBufferSurface->Resize(mDevice.Get(), Width, Height);
    mBackBufferWidth = Width;
    mBackBufferHeight = Height;
}

void FRenderer::Terminate() {
    ReleaseScenes();
    mAssetResources.Reset();
    mAssetRegistry = nullptr;

    if (mDeviceContext != nullptr) {
        mDeviceContext->ClearState();
    }

    mOverLayRenderer.Reset();
    mSceneRenderer.Reset();
    mFrameSerial = 0;

    for (FFrameResource& FrameResource : mFrameResources) {
        FrameResource.Reset();
    }

#if EnableFrameResourceFence
    mFrameFence.Reset();
    mFenceContext.Reset();

    if (mFrameFenceEvent != nullptr) {
        CloseHandle(mFrameFenceEvent);
        mFrameFenceEvent = nullptr;
    }

    mNextFenceValue = 1;
    mCompletionValues = {};
    mNextFrameResourceIndex = 0;
#endif

    mCurrentFrameResource = nullptr;

    if (mBackBufferSurface != nullptr) {
        mBackBufferSurface->Reset();
    }

    mBackBufferSurface.reset();
    mSwapChain.Reset();
}

void FRenderer::ReportLiveObjects() const {
#ifdef _DEBUG
    mDebugInterface->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
#endif
}

void FRenderer::CreateDeviceAndSwapChain(HWND WindowHandle) {
    D3D_FEATURE_LEVEL FeatureLevels[]{D3D_FEATURE_LEVEL_11_0};

    DXGI_SWAP_CHAIN_DESC SwapChainDesc{};

    SwapChainDesc.BufferDesc.Width = mBackBufferWidth;
    SwapChainDesc.BufferDesc.Height = mBackBufferHeight;
    SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = 2;
    SwapChainDesc.OutputWindow = WindowHandle;
    SwapChainDesc.Windowed = TRUE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    UINT DeviceFlags{D3D11_CREATE_DEVICE_BGRA_SUPPORT};
#ifdef _DEBUG
    DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    //디바이스 생성이 복잡해진 이유: 외장그래픽 선택 코드
    HRESULT Result{E_FAIL};
    Microsoft::WRL::ComPtr<IDXGIFactory6> Factory{};

    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(Factory.GetAddressOf())))) {
        for (UINT Index{};; ++Index) {
            Microsoft::WRL::ComPtr<IDXGIAdapter1> Adapter{};

            if (FAILED(Factory->EnumAdapterByGpuPreference(Index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(Adapter.GetAddressOf()))))
                break;

            DXGI_ADAPTER_DESC1 Description{};

            if (FAILED(Adapter->GetDesc1(&Description)) || (Description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
                continue;

            Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Using GPU: %ls", Description.Description);

            Result = D3D11CreateDeviceAndSwapChain(Adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, DeviceFlags, FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION, &SwapChainDesc, mSwapChain.ReleaseAndGetAddressOf(), mDevice.ReleaseAndGetAddressOf(), nullptr, mDeviceContext.ReleaseAndGetAddressOf());

            if (SUCCEEDED(Result))
                break;
        }
    }

    if (FAILED(Result)) {
        Result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, DeviceFlags, FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION, &SwapChainDesc, mSwapChain.ReleaseAndGetAddressOf(), mDevice.ReleaseAndGetAddressOf(), nullptr, mDeviceContext.ReleaseAndGetAddressOf());
    }

    ErrorHandler::ReportHRESULT(Result, "[ FRenderer ]", "Failed to create Direct3D device and swap chain.", ErrorHandler::EErrorLevel::Critical);
    mSwapChain->GetDesc(&SwapChainDesc);

    Microsoft::WRL::ComPtr<IDXGIDevice1> DxgiDevice{};

    if (SUCCEEDED(mDevice.As(&DxgiDevice))) {
        const HRESULT LatencyResult{DxgiDevice->SetMaximumFrameLatency(1)};

        ErrorHandler::ReportHRESULT(LatencyResult, "[ FRenderer ]", "Failed to set the maximum frame latency.", ErrorHandler::EErrorLevel::Warning);

        const HRESULT PriorityResult{DxgiDevice->SetGPUThreadPriority(0)};

        ErrorHandler::ReportHRESULT(PriorityResult, "[ FRenderer ]", "Failed to restore the default GPU thread priority.", ErrorHandler::EErrorLevel::Warning);
    }
}

bool FRenderer::CreateSamplerStates() {
    bool Succeeded{true};

    auto CreateSampler{[this](std::size_t Slot, const D3D11_SAMPLER_DESC& Description, const char* Name) {
        const HRESULT Result{mDevice->CreateSamplerState(&Description, mSamplerStates[Slot].ReleaseAndGetAddressOf())};

        ErrorHandler::ReportHRESULT(Result, "[FRenderer]", std::string("Failed to create ") + Name + " sampler.", ErrorHandler::EErrorLevel::Critical);

        return SUCCEEDED(Result);
    }};

    auto MakeDescription{[](D3D11_FILTER Filter, D3D11_TEXTURE_ADDRESS_MODE AddressMode) {
        D3D11_SAMPLER_DESC Description{};

        Description.Filter = Filter;
        Description.AddressU = AddressMode;
        Description.AddressV = AddressMode;
        Description.AddressW = AddressMode;
        Description.MipLODBias = 0.0f;
        Description.MaxAnisotropy = Filter == D3D11_FILTER_ANISOTROPIC ? 8 : 1;
        Description.ComparisonFunc = D3D11_COMPARISON_NEVER;
        Description.MinLOD = 0.0f;
        Description.MaxLOD = D3D11_FLOAT32_MAX;

        return Description;
    }};

    Succeeded = CreateSampler(0, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP), "LinearWrap") && Succeeded;
    Succeeded = CreateSampler(1, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP), "LinearClamp") && Succeeded;
    Succeeded = CreateSampler(2, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP), "PointClamp") && Succeeded;
    Succeeded = CreateSampler(3, MakeDescription(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP), "PointWrap") && Succeeded;
    Succeeded = CreateSampler(4, MakeDescription(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP), "AnisotropicWrap") && Succeeded;

    D3D11_SAMPLER_DESC ShadowDescription{MakeDescription(D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_BORDER)};

    ShadowDescription.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    ShadowDescription.BorderColor[0] = 1.0f;
    ShadowDescription.BorderColor[1] = 1.0f;
    ShadowDescription.BorderColor[2] = 1.0f;
    ShadowDescription.BorderColor[3] = 1.0f;
    Succeeded = CreateSampler(5, ShadowDescription, "ShadowCompare") && Succeeded;

    return Succeeded;
}

ID3D11ShaderResourceView* FRenderer::GetTextureResource(FAssetHandle Handle) {
    const UTexture* Texture{mAssetRegistry != nullptr ? mAssetRegistry->ResolveAsset<UTexture>(Handle) : nullptr};

    return Texture != nullptr ? mAssetResources.GetTexture(*Texture) : nullptr;
}

bool FRenderer::PrepareAssetResources() {
    if (mAssetRegistry == nullptr) {
        return false;
    }

    bool Complete{true};

    for (FAssetHandle Handle : mAssetRegistry->GetAssetHandles(*UTexture::StaticTypeInfo())) {
        const UTexture* Texture{mAssetRegistry->ResolveAsset<UTexture>(Handle)};

        Complete = Texture != nullptr && mAssetResources.GetTexture(*Texture) != nullptr && Complete;
    }

    for (FAssetHandle Handle : mAssetRegistry->GetAssetHandles(*UMesh::StaticTypeInfo())) {
        const UMesh* Mesh{mAssetRegistry->ResolveAsset<UMesh>(Handle)};

        Complete = Mesh != nullptr && mAssetResources.GetMesh(*Mesh) != nullptr && Complete;
    }

    for (FAssetHandle Handle : mAssetRegistry->GetAssetHandles(*UPipeline::StaticTypeInfo())) {
        const UPipeline* Pipeline{mAssetRegistry->ResolveAsset<UPipeline>(Handle)};

        Complete = Pipeline != nullptr && mAssetResources.GetPipeline(*Pipeline) != nullptr && Complete;
    }

    return Complete;
}
