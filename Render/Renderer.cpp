#include "pch.h"

#include "Renderer.h"
#include "../ErrorHandler.h"

#include "Pipeline/UPipeline.h"
#include "../Core/Asset/UTexture.h"

#include <ranges>
#include <range/v3/view/chunk_by.hpp>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

#include "../Core/Console/Console.h"

FRenderer::~FRenderer() = default;

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
    if (!CreateSamplerStates() || !mModelContextArray.Initialize(mDevice.Get(), mDeviceContext.Get(), 128) || !mLightContextArray.Initialize(mDevice.Get(), mDeviceContext.Get(), 16) || !mRootConstants.Initialize(mDevice.Get()) || !mTextRenderer.Initialize(mDevice.Get(), 256) || !mBillboardRenderer.Initialize(mDevice.Get(), 64)) {
        return false;
    }

    mFrameContexts.reserve(128);
    return true;
}

void FRenderer::BeginUiRender() {
    mBackBufferSurface->Bind(mDeviceContext.Get());
    mBackBufferSurface->Clear(mDeviceContext.Get(), UiClearColor);
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
    mSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

ID3D11Device* FRenderer::GetDevice() const {
    return mDevice.Get();
}

ID3D11DeviceContext* FRenderer::GetDeviceContext() const {
    return mDeviceContext.Get();
}

void FRenderer::BindAssetRegistry(FAssetRegistry* InAssetRegistry) {
    mAssetRegistry = InAssetRegistry;
}

void FRenderer::RenderScene(IRenderSurface& Target, FRenderProbe& Probe, const CameraProbe& Camera, const FRenderSettings& Settings) {
    const float ClearColor[4]{Settings.mClearColor.mX, Settings.mClearColor.mY, Settings.mClearColor.mZ, Settings.mClearColor.mW};
    Target.Bind(mDeviceContext.Get());
    Target.Clear(mDeviceContext.Get(), ClearColor);

    if (!UploadLightContext(Probe)) {
        return;
    }

    if (Probe.mBForceUnlit) {
        for (FActorProbe& ActorProbe : Probe.mActorProbes) {
            ActorProbe.mFlags |= static_cast<Uint32>(ERenderObjectFlags::Unlit);
        }
    }

    if (mAssetRegistry != nullptr) {
        mAssetRegistry->GetMaterialBuffer().Flush(mDeviceContext.Get());
    }
    RenderActorList(Probe.mActorProbes, Camera, false, Settings.mBRenderSky);
    RenderOutline(Probe.mActorProbes, Camera, Settings.mBRenderSky);
}

bool FRenderer::UploadLightContext(const FRenderProbe& Probe) {
    if (!mLightContextArray.UploadDiscard(mDevice.Get(), mDeviceContext.Get(), Probe.mLightProbes)) {
        return false;
    }

    mFrameLightCount = mLightContextArray.GetCount();
    mDeviceContext->PSSetShaderResources(2, 1, mLightContextArray.GetSRV());
    return true;
}

void FRenderer::RenderGizmos(IRenderSurface& Target, FRenderProbe& Probe, const CameraProbe& Camera) {
    if (Probe.mGizmoProbes.empty()) {
        return;
    }

    Target.ClearDepth(mDeviceContext.Get());

    RenderActorList(Probe.mGizmoProbes, Camera);
}

void FRenderer::RenderOutline(const TArray<FActorProbe>& ActorProbes, const CameraProbe& Camera, bool BRenderSky) {
    TArray<FActorProbe> OutlineProbes{};

    for (const FActorProbe& ActorProbe : ActorProbes) {
        if ((ActorProbe.mFlags & static_cast<Uint32>(ERenderObjectFlags::Selected)) != 0) {
            OutlineProbes.push_back(ActorProbe);
        }
    }

    if (OutlineProbes.empty()) {
        return;
    }

    RenderActorList(OutlineProbes, Camera, true, BRenderSky);
}

void FRenderer::RenderActorList(TArray<FActorProbe>& ActorProbes, const CameraProbe& Camera, bool BOutline, bool BRenderSky) {
    if (ActorProbes.empty() || mAssetRegistry == nullptr) {
        return;
    }

    struct FDrawItem {
        FActorProbe mProbe{};
        FAssetHandle mMaterialHandle{};
        Uint32 mMaterialGroupIndex{0};
        Uint32 mFirstIndex{0};
        Uint32 mIndexCount{0};
    };

    const FAssetHandle SkyPipelineHandle{mAssetRegistry->FindAsset(FAssetPath{"/Game/Pipeline/SkyDome.json"})};
    TArray<FDrawItem> DrawItems{};
    for (const FActorProbe& Probe : ActorProbes) {
        if (!BRenderSky && Probe.mPipelineHandle == SkyPipelineHandle) {
            continue;
        }

        UMesh* Mesh{mAssetRegistry->ResolveAsset<UMesh>(Probe.mMeshHandle)};
        if (Mesh == nullptr || mAssetRegistry->ResolveAsset<UPipeline>(Probe.mPipelineHandle) == nullptr) {
            continue;
        }

        const auto AddDrawItem{[&DrawItems, &Probe, this](FAssetHandle MaterialHandle, Uint32 MaterialGroupIndex, Uint32 FirstIndex, Uint32 IndexCount) {
            UMaterial* Material{mAssetRegistry->ResolveAsset<UMaterial>(MaterialHandle)};
            if (Material == nullptr || IndexCount == 0) {
                return;
            }

            const Uint32 ResolvedGroupIndex{Material->GetGPUIndex(MaterialGroupIndex) != UINT32_MAX ? MaterialGroupIndex : 0u};
            if (Material->GetGPUIndex(ResolvedGroupIndex) == UINT32_MAX) {
                return;
            }

            DrawItems.push_back(FDrawItem{ .mProbe = Probe, .mMaterialHandle = MaterialHandle, .mMaterialGroupIndex = ResolvedGroupIndex, .mFirstIndex = FirstIndex, .mIndexCount = IndexCount});
        }};

        const TArray<UMesh::FSubMesh>& SubMeshes{Mesh->GetSubMeshes()};
        if (SubMeshes.empty()) {
            AddDrawItem(Probe.mMaterialHandle, 0, 0, static_cast<Uint32>(Mesh->GetIndices().size()));
            continue;
        }

        for (const UMesh::FSubMesh& SubMesh : SubMeshes) {
            AddDrawItem(Probe.mMaterialHandle, SubMesh.mMaterialGroupIndex, SubMesh.mFirstIndex, SubMesh.mIndexCount);
        }
    }

    if (DrawItems.empty()) {
        return;
    }

    auto GetRenderChunkKey{[this](const FDrawItem& Data) {
        const FMaterialChunkSignature Signature{mAssetRegistry->ResolveAsset<UMaterial>(Data.mMaterialHandle)->BuildChunkSignature(Data.mMaterialGroupIndex)};
        return TTuple{ Data.mProbe.mPipelineHandle.mId, Data.mProbe.mPipelineHandle.mGeneration, Signature.mTextureFieldCount, Signature.mTextureHandles, Data.mProbe.mMeshHandle.mId, Data.mProbe.mMeshHandle.mGeneration, Data.mMaterialHandle.mId, Data.mMaterialHandle.mGeneration, Data.mMaterialGroupIndex, Data.mFirstIndex, Data.mIndexCount};
    }};

    std::ranges::sort(DrawItems, {}, GetRenderChunkKey);

    auto Groups{DrawItems | ranges::views::chunk_by([&GetRenderChunkKey](const FDrawItem& A, const FDrawItem& B) {
                    return GetRenderChunkKey(A) == GetRenderChunkKey(B);
                })};

    mFrameContexts.clear();
    mFrameContexts.reserve(DrawItems.size());

    std::ranges::transform(Groups | std::views::join, std::back_inserter(mFrameContexts), [&](const auto& AC) {
        return ModelContext{ .mWorld = AC.mProbe.mWorld, .mMaterialIndex = mAssetRegistry->ResolveAsset<UMaterial>(AC.mMaterialHandle)->GetGPUIndex(AC.mMaterialGroupIndex), .mFlags = AC.mProbe.mFlags};
    });

    ID3D11ShaderResourceView* NullModelContext{nullptr};
    mDeviceContext->VSSetShaderResources(0, 1, &NullModelContext);
    mDeviceContext->PSSetShaderResources(0, 1, &NullModelContext);
    if (!mModelContextArray.UploadDiscard(mDevice.Get(), mDeviceContext.Get(), mFrameContexts)) {
        return;
    }

    mDeviceContext->VSSetShaderResources(0, 1, mModelContextArray.GetSRV());
    mDeviceContext->PSSetShaderResources(0, 1, mModelContextArray.GetSRV());

    mDeviceContext->VSSetShaderResources(1, 1, mAssetRegistry->GetMaterialBuffer().GetSRV());
    mDeviceContext->PSSetShaderResources(1, 1, mAssetRegistry->GetMaterialBuffer().GetSRV());

    struct CameraData {
        FMatrix mView{};
        FMatrix mProjection{};
        FMatrix mViewProjection{};
    };

    mRootConstants.SetGraphicsRoot32BitConstants(CameraData{ .mView = Camera.mView, .mProjection = Camera.mProjection, .mViewProjection = Camera.mViewProjection}, 0);

    mRootConstants.SetGraphicsRoot32BitConstant(mFrameLightCount, 49);
    mRootConstants.Bind(mDeviceContext.Get(), 0, EGraphicsShaderStage::Graphics);
    Uint32 InstanceCount{0};
    FMaterialChunkSignature BoundTextureSet{};
    bool BTextureSetBound{false};

    BindSamplerStates();

    for (auto G : Groups) {
        const FDrawItem& First{G.front()};
        const FMaterialChunkSignature Signature{mAssetRegistry->ResolveAsset<UMaterial>(First.mMaterialHandle)->BuildChunkSignature(First.mMaterialGroupIndex)};

        UPipeline* Pipeline{mAssetRegistry->ResolveAsset<UPipeline>(First.mProbe.mPipelineHandle)};
        if (BOutline) {
            Pipeline->SetRenderMode(ERenderMode::Outline);
        }

        UMesh* Mesh{mAssetRegistry->ResolveAsset<UMesh>(First.mProbe.mMeshHandle)};

        const bool BLitWireframe{!BOutline && Pipeline->GetRenderMode() == ERenderMode::LitWireframe};
        if (BLitWireframe) {
            Pipeline->Bind(mDeviceContext.Get(), ERenderMode::Lit);
        } else {
            Pipeline->Bind(mDeviceContext.Get());
        }

        if (!BTextureSetBound || BoundTextureSet != Signature) {
            std::array<ID3D11ShaderResourceView*, MaxMaterialTextureFields> TextureSRVs{};

            for (Uint8 TextureFieldIndex{0}; TextureFieldIndex < Signature.mTextureFieldCount; ++TextureFieldIndex) {
                UTexture* Texture{mAssetRegistry->ResolveAsset<UTexture>(Signature.GetTextureHandle(TextureFieldIndex))};
                TextureSRVs[TextureFieldIndex] = Texture != nullptr ? Texture->GetSRV() : nullptr;
            }

            mDeviceContext->PSSetShaderResources(3, static_cast<UINT>(TextureSRVs.size()), TextureSRVs.data());
            mDeviceContext->VSSetShaderResources(3, static_cast<UINT>(TextureSRVs.size()), TextureSRVs.data());

            BoundTextureSet = Signature;
            BTextureSetBound = true;
        }

        ID3D11Buffer* VertexBuffers[]{ Mesh->GetVertexBuffer(EVertexAttribute::Position), Mesh->GetVertexBuffer(EVertexAttribute::Normal), Mesh->GetVertexBuffer(EVertexAttribute::UV), Mesh->GetVertexBuffer(EVertexAttribute::Color)};

        Uint32 Strides[]{ Mesh->GetVertexStride(EVertexAttribute::Position), Mesh->GetVertexStride(EVertexAttribute::Normal), Mesh->GetVertexStride(EVertexAttribute::UV), Mesh->GetVertexStride(EVertexAttribute::Color)};

        Uint32 Offsets[]{0, 0, 0, 0};

        ID3D11Buffer* IndexBuffer{Mesh->GetIndexBuffer()};

        mDeviceContext->IASetVertexBuffers(0, _countof(VertexBuffers), VertexBuffers, Strides, Offsets);
        mDeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

        mRootConstants.SetGraphicsRoot32BitConstant(InstanceCount, 48);

        if (ImGui::GetCurrentContext() != nullptr) {
            auto& Io{ImGui::GetIO()};
            float DT{Io.DeltaTime};
            mCountTime += DT;
            if (mCountTime >= 0.1f) {
                mCurrentFrame = (mCurrentFrame + 1) % 250;
                mCountTime = 0.f;
            }
        }

        mRootConstants.SetGraphicsRoot32BitConstant(mCurrentFrame, 50);

        mRootConstants.Commit(mDeviceContext.Get());

        mDeviceContext->DrawIndexedInstanced(First.mIndexCount, static_cast<Uint32>(G.size()), First.mFirstIndex, 0, 0);
        if (BLitWireframe) {
            Pipeline->Bind(mDeviceContext.Get(), ERenderMode::LitWireframe);
            mDeviceContext->DrawIndexedInstanced(First.mIndexCount, static_cast<Uint32>(G.size()), First.mFirstIndex, 0, 0);
        }

        InstanceCount += static_cast<Uint32>(G.size());
    }
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
    mDeviceContext->ClearState();

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

#ifdef _DEBUG
    ErrorHandler::ReportHRESULT(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                                              D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
                                                              FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION,
                                                              &SwapChainDesc, &mSwapChain, &mDevice, nullptr, &mDeviceContext),
                                "[ FRenderer ]", "Failed to create Direct3D device and swap chain.", ErrorHandler::EErrorLevel::Critical);
#else
    ErrorHandler::ReportHRESULT(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                                              D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                                              FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION,
                                                              &SwapChainDesc, &mSwapChain, &mDevice, nullptr, &mDeviceContext),
                                "[ FRenderer ]", "Failed to create Direct3D device and swap chain.", ErrorHandler::EErrorLevel::Critical);
#endif
    mSwapChain->GetDesc(&SwapChainDesc);
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

void FRenderer::RenderText(const FRenderProbe& Probe, const CameraProbe& Camera) {
    if (mAssetRegistry != nullptr) {
        mTextRenderer.Render(mDeviceContext.Get(), Probe.mTextProbes, Camera, mAssetRegistry);
        mBillboardRenderer.Render(mDeviceContext.Get(), Probe.mBillboardProbes, Camera, mAssetRegistry);
    }
}
