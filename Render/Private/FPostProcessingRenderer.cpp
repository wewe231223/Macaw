#include "pch.h"
#include "Render/FPostProcessingRenderer.h"

bool FPostProcessingRenderer::Initialize(ID3D11Device* Device) {
    Reset();

    if (Device == nullptr) {
        return false;
    }

    const FShaderDescription VertexDescription{"./Content/Shader/PostProcessing.hlsl", "MainVS", "vs_5_0", EShaderStage::Vertex};
    const FShaderDescription PixelDescription{"./Content/Shader/PostProcessing.hlsl", "MainPS", "ps_5_0", EShaderStage::Pixel};

    if (!mVertexShader.Initialize(Device, VertexDescription) || !mPixelShader.Initialize(Device, PixelDescription)) {
        Reset();
        return false;
    }

    D3D11_RASTERIZER_DESC RasterizerDescription{};

    RasterizerDescription.FillMode = D3D11_FILL_SOLID;
    RasterizerDescription.CullMode = D3D11_CULL_NONE;
    RasterizerDescription.DepthClipEnable = true;

    D3D11_BLEND_DESC BlendDescription{};

    BlendDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    BlendDescription.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    BlendDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BlendDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    D3D11_DEPTH_STENCIL_DESC DepthDescription{};

    DepthDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    DepthDescription.DepthFunc = D3D11_COMPARISON_ALWAYS;

    D3D11_SAMPLER_DESC SamplerDescription{};

    SamplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SamplerDescription.MaxLOD = D3D11_FLOAT32_MAX;

    FGraphicsBufferDescription BufferDescription{};

    BufferDescription.mByteSize = sizeof(FVector4);
    BufferDescription.mUsage = D3D11_USAGE_DYNAMIC;
    BufferDescription.mBindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDescription.mCpuAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(Device->CreateRasterizerState(&RasterizerDescription, mRasterizerState.GetAddressOf())) || FAILED(Device->CreateBlendState(&BlendDescription, mBlendState.GetAddressOf())) || FAILED(Device->CreateDepthStencilState(&DepthDescription, mDepthStencilState.GetAddressOf())) || FAILED(Device->CreateSamplerState(&SamplerDescription, mSamplerState.GetAddressOf())) || !mConstants.Initialize(Device, BufferDescription)) {
        Reset();
        return false;
    }

    return true;
}

bool FPostProcessingRenderer::Render(ID3D11DeviceContext* Context, ID3D11ShaderResourceView* SceneColor, const IRenderSurface& Target, const FPostProcessingSettings& Settings, bool Enabled) {
    if (Context == nullptr || SceneColor == nullptr || !Target.IsValid() || !mConstants.IsValid()) {
        return false;
    }

    const D3D11_VIEWPORT& Viewport{Target.GetViewport()};
    const FVector4 Parameters{1.0f / Viewport.Width, 1.0f / Viewport.Height, Enabled && Settings.mFXAA ? 1.0f : 0.0f, 0.0f};

    if (!mConstants.WriteDiscard(Context, &Parameters, sizeof(Parameters))) {
        return false;
    }

    Target.Bind(Context, nullptr);
    Context->IASetInputLayout(nullptr);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context->VSSetShader(mVertexShader.GetVertexShader(), nullptr, 0);
    Context->PSSetShader(mPixelShader.GetPixelShader(), nullptr, 0);
    Context->GSSetShader(nullptr, nullptr, 0);
    Context->HSSetShader(nullptr, nullptr, 0);
    Context->DSSetShader(nullptr, nullptr, 0);
    Context->RSSetState(mRasterizerState.Get());
    Context->OMSetBlendState(mBlendState.Get(), nullptr, 0xffffffff);
    Context->OMSetDepthStencilState(mDepthStencilState.Get(), 0);

    ID3D11Buffer* Constants{mConstants.GetBuffer()};
    ID3D11SamplerState* Sampler{mSamplerState.Get()};

    Context->PSSetConstantBuffers(0, 1, &Constants);
    Context->PSSetSamplers(0, 1, &Sampler);
    Context->PSSetShaderResources(0, 1, &SceneColor);
    Context->DrawInstanced(3, 1, 0, 0);

    ID3D11ShaderResourceView* NullResource{};

    Context->PSSetShaderResources(0, 1, &NullResource);

    return true;
}

void FPostProcessingRenderer::Reset() {
    mVertexShader.Reset();
    mPixelShader.Reset();
    mConstants.Reset();
    mRasterizerState.Reset();
    mBlendState.Reset();
    mDepthStencilState.Reset();
    mSamplerState.Reset();
}
