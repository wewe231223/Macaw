#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/FRenderStats.h"
#include "RenderCore/IRendererModule.h"
#include "Renderer/FMeshResource.h"
#include "Renderer/FLineVertex.h"

#include <d3d11.h>
#include <wrl/client.h>

class FRenderer final : public IRendererModule {
public:
    ReflectTypes(FRenderer, IRendererModule);

public:
    FRenderer();
    ~FRenderer() override;
    FRenderer(const FRenderer&) = delete;
    FRenderer& operator=(const FRenderer&) = delete;

public:
    ReflectStruct(FRenderer);

    bool Initialize(void* WindowHandle, Uint32 Width, Uint32 Height) override;
    void Shutdown() override;

    std::unique_ptr<FSceneInterface> AllocateScene() override;
    bool UploadMesh(EPrimitiveType Type, const FMeshData& Mesh) override;

    std::unique_ptr<FRenderTarget> CreateRenderTarget(Uint32 Width, Uint32 Height) override;
    bool ResizeRenderTarget(FRenderTarget& Target, Uint32 Width, Uint32 Height) override;

    void BeginFrame(Uint32 Width, Uint32 Height) override;
    void BeginRenderingViewFamily(const FSceneViewFamily& Family) override;

    void BeginUiRender() override;
    void Present() override;

    void* GetNativeDevice() const override;
    void* GetNativeContext() const override;
    FRenderStats GetStats() const override;

private:
    bool CreateBackBuffer(Uint32 Width, Uint32 Height);
    bool CreatePipeline();
    bool Compile(const char* Source, std::size_t Size, const char* Entry, const char* Target, Microsoft::WRL::ComPtr<ID3DBlob>& Code);
    void DrawLines(const FPrimitiveDrawInterface& Elements, bool DepthTest);

private:
    ReflectMembers();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice{};
    ReflectProperty(mDevice, "Device", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
    ReflectProperty(mSwapChain, "SwapChain", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mBackBuffer{};
    ReflectProperty(mBackBuffer, "BackBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Microsoft::WRL::ComPtr<ID3D11VertexShader> mMeshVertexShader{};
    ReflectProperty(mMeshVertexShader, "MeshVertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11PixelShader> mMeshPixelShader{};
    ReflectProperty(mMeshPixelShader, "MeshPixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11InputLayout> mMeshLayout{};
    ReflectProperty(mMeshLayout, "MeshLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Microsoft::WRL::ComPtr<ID3D11VertexShader> mLineVertexShader{};
    ReflectProperty(mLineVertexShader, "LineVertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11PixelShader> mLinePixelShader{};
    ReflectProperty(mLinePixelShader, "LinePixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11InputLayout> mLineLayout{};
    ReflectProperty(mLineLayout, "LineLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Microsoft::WRL::ComPtr<ID3D11Buffer> mViewBuffer{};
    ReflectProperty(mViewBuffer, "ViewBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11Buffer> mLineBuffer{};
    ReflectProperty(mLineBuffer, "LineBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
    ReflectProperty(mRasterizer, "Rasterizer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthEnabled{};
    ReflectProperty(mDepthEnabled, "DepthEnabled", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthReadOnly{};
    ReflectProperty(mDepthReadOnly, "DepthReadOnly", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthDisabled{};
    ReflectProperty(mDepthDisabled, "DepthDisabled", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::array<FMeshResource, 3> mMeshes{};
    ReflectProperty(mMeshes, "Meshes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    TArray<FLineVertex> mLineVertices{};
    ReflectProperty(mLineVertices, "LineVertices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mLineCapacity{};
    ReflectProperty(mLineCapacity, "LineCapacity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Uint32 mWidth{};
    ReflectProperty(mWidth, "Width", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mHeight{};
    ReflectProperty(mHeight, "Height", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FRenderStats mStats{};
    ReflectProperty(mStats, "Stats", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
