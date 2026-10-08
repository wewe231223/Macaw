#pragma once

#include "Render/Buffer/FGraphicsBuffer.h"
#include "Render/Pipeline/FShader.h"
#include "Render/IRenderSurface.h"
#include "RenderCore/FRenderData.h"

class FPostProcessingRenderer {
public:
    bool Initialize(ID3D11Device* Device);
    bool Render(ID3D11DeviceContext* Context, ID3D11ShaderResourceView* SceneColor, const IRenderSurface& Target, const FPostProcessingSettings& Settings, bool Enabled);
    void Reset();

private:
    FShader mVertexShader{};
    FShader mPixelShader{};
    FGraphicsBuffer mConstants{};
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
    Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSamplerState{};
};
