#pragma once

#include "Render/Pipeline/FShader.h"
#include "Asset/Pipeline/UPipeline.h"

class FPipelineRenderResource {
private:
    struct FPipelineState {
        FShader mVertexShader{};
        FShader mPixelShader{};
        FShader mGeometryShader{};
        Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
        Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
        D3D11_PRIMITIVE_TOPOLOGY mPrimitiveTopology{D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST};
        bool mInitialized{false};
    };

public:
    FPipelineRenderResource() = default;
    ~FPipelineRenderResource() = default;

    FPipelineRenderResource(const FPipelineRenderResource&) = delete;
    FPipelineRenderResource& operator=(const FPipelineRenderResource&) = delete;
    FPipelineRenderResource(FPipelineRenderResource&&) noexcept = default;
    FPipelineRenderResource& operator=(FPipelineRenderResource&&) noexcept = default;

public:
    bool Initialize(ID3D11Device* Device, const UPipeline& Pipeline);
    void Bind(ID3D11DeviceContext* Context, ERenderMode Mode, UINT StencilReference = 1) const;
    void Reset();

private:
    bool Make(ID3D11Device* Device, const FPipelineDescription& Description, FPipelineState& Pipeline);

private:
    std::vector<FPipelineState> mPipelines{};
};
