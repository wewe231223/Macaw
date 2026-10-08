#pragma once
#include <d3d11.h>
#include <wrl/client.h>

struct FMeshDrawPipelineState {
    bool IsValid() const;
    void Bind(ID3D11DeviceContext* Context, UINT StencilReference) const;
    bool operator==(const FMeshDrawPipelineState& Other) const;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> mGeometryShader{};
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout{};
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizerState{};
    Microsoft::WRL::ComPtr<ID3D11BlendState> mBlendState{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthStencilState{};
    D3D11_PRIMITIVE_TOPOLOGY mPrimitiveTopology{D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST};
};
