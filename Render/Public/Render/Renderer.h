#pragma once
#include "Render/RenderConfig.h"

#if EnableFrameResourceFence
#include <d3d11_4.h>
#else
#include <d3d11.h>
#endif
#include <wrl/client.h>
#include <array>
#include <memory>
#include "Render/FRenderAssetResources.h"
#include "Render/FRenderView.h"
#include "Render/FSceneRenderer.h"
#include "Render/FOverLayRenderer.h"
#include "Render/FSwapChainRenderSurface.h"
#include "Render/FFrameResource.h"
#include "RenderCore/FSceneInterface.h"

class FRendererScene;
struct FSceneUpdateBatch;

class FRenderer {
public:
    FRenderer() = default;
    ~FRenderer();

    FRenderer(const FRenderer&) = delete;
    FRenderer& operator=(const FRenderer&) = delete;
    FRenderer(FRenderer&&) = delete;
    FRenderer& operator=(FRenderer&&) = delete;

public:
    void Create(HWND WindowHandle, UINT Width, UINT Height);
    bool Initialize();

    void BeginFrame();

    std::weak_ptr<FSceneInterface> CreateScene();
    bool ReleaseScene(FSceneHandle Handle);
    void ResetScenes();
    FSceneHandle GetSceneHandle(Uint64 SceneId) const;
    const FRenderScene* FindScene(FSceneHandle Handle) const;

    void RenderView(const FRenderView& View, const FRenderScene& Scene, const FOverlayRenderData& Overlay = {});

    void BeginUiRender();
    void EndFrame();

    ID3D11Device* GetDevice() const;
    ID3D11DeviceContext* GetDeviceContext() const;

    bool BindAssetRegistry(const IAssetRegistry* InAssetRegistry, IAssetRegistryMutator* InAssetRegistryMutator = nullptr);
    bool PrepareAssetResources();

    ID3D11ShaderResourceView* GetTextureResource(FAssetHandle Handle);

    void ReSize(Uint32 Width, Uint32 Height);
    void Terminate();
    void ReportLiveObjects() const;

private:
    friend class FRendererScene;

    bool ApplySceneUpdates(FSceneUpdateBatch& Updates);
    void ReleaseSceneResources(Uint64 SceneId);
    void ReleaseScenes();

    void CreateDeviceAndSwapChain(HWND WindowHandle);
    bool CreateSamplerStates();

    void BindSamplerStates();

private:
#if EnableFrameResourceFence
    static constexpr Uint32 mFrameResourceCount{3};
#else
    static constexpr Uint32 mFrameResourceCount{1};
#endif

#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D11Debug> mDebugInterface{};
#endif
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice{};
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext{};

#if EnableFrameResourceFence
    Microsoft::WRL::ComPtr<ID3D11DeviceContext4> mFenceContext{};
    Microsoft::WRL::ComPtr<ID3D11Fence> mFrameFence{};
    HANDLE mFrameFenceEvent{nullptr};
    Uint64 mNextFenceValue{1};
    std::array<Uint64, mFrameResourceCount> mCompletionValues{};
    Uint32 mNextFrameResourceIndex{};
#endif

    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
    std::unique_ptr<FSwapChainRenderSurface> mBackBufferSurface{};

    // s0: LinearWrap, s1: LinearClamp, s2: PointClamp, s3: PointWrap, s4: AnisotropicWrap, s5: ShadowCompare.
    std::array<Microsoft::WRL::ComPtr<ID3D11SamplerState>, 6> mSamplerStates{};

    const IAssetRegistry* mAssetRegistry{nullptr};
    FRenderAssetResources mAssetResources{};

    std::array<FFrameResource, mFrameResourceCount> mFrameResources{};
    FFrameResource* mCurrentFrameResource{nullptr};

    Uint64 mFrameSerial{};
    TMap<Uint64, std::shared_ptr<FRendererScene>> mRenderScenes{};

    FSceneRenderer mSceneRenderer{};
    FOverLayRenderer mOverLayRenderer{};

    const float mUiClearColor[4]{0.2f, 0.2f, 0.7f, 1.0f};
    Uint32 mBackBufferWidth{};
    Uint32 mBackBufferHeight{};
};
