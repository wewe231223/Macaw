#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <array>
#include <memory>

#include "../Core/Base/FRenderProbe.h"
#include "../Core/Asset/FAssetRegistry.h"

#include "Pipeline/UPipeline.h"
#include "../Core/Asset/UMaterial.h"
#include "../Core/Asset/UMesh.h"

#include "../Core/Buffer/TGraphicsArray.h"
#include "../Core/Buffer/TGraphicsRootConstants.h"

#include "FTextRenderer.h"
#include "FSceneRenderSurface.h"

#include "../../Scene/FWorldEditorContext.h"

#include "FBillboardRenderer.h"

class FRenderer {
    struct ModelContext { FMatrix mWorld{}; Uint32 mMaterialIndex{UINT32_MAX}; Uint32 mFlags{0x0000'0000}; };

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

    void BeginUiRender();
    void RenderScene(IRenderSurface& Target, FRenderProbe& Probe, const CameraProbe& Camera, const FRenderSettings& Settings);
    void RenderGizmos(IRenderSurface& Target, FRenderProbe& Probe, const CameraProbe& Camera);
    void RenderOutline(const TArray<FActorProbe>& ActorProbes, const CameraProbe& Camera, bool BRenderSky = true);
    void RenderText(const FRenderProbe& Probe, const CameraProbe& Camera);
    void RenderActorList(TArray<FActorProbe>& ActorProbes, const CameraProbe& Camera, bool BOutline = false, bool BRenderSky = true);
    void EndFrame();

    ID3D11Device* GetDevice() const;
    ID3D11DeviceContext* GetDeviceContext() const;

    void BindAssetRegistry(FAssetRegistry* InAssetRegistry);

    void ReSize(Uint32 Width, Uint32 Height);

    void Terminate();
    void ReportLiveObjects() const;

private:
    void CreateDeviceAndSwapChain(HWND WindowHandle);

    bool CreateSamplerStates();
    void BindSamplerStates();
    bool UploadLightContext(const FRenderProbe& Probe);

private:
#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D11Debug> mDebugInterface{};
#endif
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice{};
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext{};

    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};

    std::unique_ptr<IRenderSurface> mBackBufferSurface{};

    // s0: LinearWrap, s1: LinearClamp, s2: PointClamp, s3: PointWrap, s4: AnisotropicWrap, s5: ShadowCompare.
    std::array<Microsoft::WRL::ComPtr<ID3D11SamplerState>, 6> mSamplerStates{};

    FAssetRegistry* mAssetRegistry{nullptr};

    TGraphicsArray<ModelContext, true, true> mModelContextArray{};
    TGraphicsArray<FLightProbe, true, true> mLightContextArray{};
    TArray<ModelContext> mFrameContexts{};
    TGraphicsRootConstants<64> mRootConstants{};

    FTextRenderer mTextRenderer{};
    FBillboardRenderer mBillboardRenderer{};

    const float UiClearColor[4]{0.2f, 0.2f, 0.7f, 1.0f};

    std::size_t mRenderIndex{0};
    Uint32 mFrameLightCount{0};
    Uint32 mBackBufferWidth{0};
    Uint32 mBackBufferHeight{0};

    //VAT current time 계산용(임시)
    float mCountTime{0.f};
    Uint32 mCurrentFrame{0};
};
