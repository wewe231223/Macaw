/**
 * @file Renderer.h
 * @brief 전역 에셋을 조회하고 장치 및 렌더 씬별 GPU 자원을 소유하는 렌더러를 정의합니다.
 */
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <memory>
#include "Render/FRenderAssetResources.h"
#include "Render/FRenderView.h"
#include "Render/FSceneRenderer.h"
#include "Render/FOverLayRenderer.h"
#include "Render/FSwapChainRenderSurface.h"
#include "Render/FRenderBuffers.h"
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
    bool Create(HWND WindowHandle, UINT Width, UINT Height);
    /**
     * @brief 렌더러 소유의 GPU 에셋 캐시와 공용 렌더 자원을 초기화합니다.
     * @return 모든 렌더 자원 초기화에 성공하면 true입니다.
     * @details 에셋 레지스트리 연결은 필요하지 않으며 실제 에셋 조회 시 활성 전역 서비스를 사용합니다.
     */
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

    /**
     * @brief 전역 레지스트리에 로드된 에셋의 GPU 자원을 준비합니다.
     * @return 활성 에셋 서비스가 존재하고 모든 GPU 자원이 준비되면 true입니다.
     */
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

    bool CreateDeviceAndSwapChain(HWND WindowHandle);
    bool CreateSamplerStates();

    void BindSamplerStates();

private:
#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D11Debug> mDebugInterface{};
#endif
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice{};
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext{};

    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
    std::unique_ptr<FSwapChainRenderSurface> mBackBufferSurface{};

    // s0: LinearWrap, s1: LinearClamp, s2: PointClamp, s3: PointWrap, s4: AnisotropicWrap, s5: ShadowCompare.
    std::array<Microsoft::WRL::ComPtr<ID3D11SamplerState>, 6> mSamplerStates{};

    FRenderAssetResources mAssetResources{};

    FRenderBuffers mRenderBuffers{};

    Uint64 mFrameSerial{};
    TMap<Uint64, std::shared_ptr<FRendererScene>> mRenderScenes{};

    FSceneRenderer mSceneRenderer{};
    FOverLayRenderer mOverLayRenderer{};

    const float mUiClearColor[4]{0.2f, 0.2f, 0.7f, 1.0f};
    Uint32 mBackBufferWidth{};
    Uint32 mBackBufferHeight{};
};
