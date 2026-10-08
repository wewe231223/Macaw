#include "pch.h"
#include "Render/FMeshDrawPipelineState.h"

bool FMeshDrawPipelineState::IsValid() const {
    return mVertexShader != nullptr && mPixelShader != nullptr;
}

bool FMeshDrawPipelineState::operator==(const FMeshDrawPipelineState& Other) const {
    return mVertexShader.Get() == Other.mVertexShader.Get() && mPixelShader.Get() == Other.mPixelShader.Get() && mGeometryShader.Get() == Other.mGeometryShader.Get() && mInputLayout.Get() == Other.mInputLayout.Get() && mRasterizerState.Get() == Other.mRasterizerState.Get() && mBlendState.Get() == Other.mBlendState.Get() && mDepthStencilState.Get() == Other.mDepthStencilState.Get() && mPrimitiveTopology == Other.mPrimitiveTopology;
}

void FMeshDrawPipelineState::Bind(ID3D11DeviceContext* Context, UINT StencilReference) const {
    Context->IASetInputLayout(mInputLayout.Get());
    Context->IASetPrimitiveTopology(mPrimitiveTopology);
    Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
    Context->PSSetShader(mPixelShader.Get(), nullptr, 0);
    Context->GSSetShader(mGeometryShader.Get(), nullptr, 0);
    Context->HSSetShader(nullptr, nullptr, 0);
    Context->DSSetShader(nullptr, nullptr, 0);
    Context->RSSetState(mRasterizerState.Get());
    Context->OMSetBlendState(mBlendState.Get(), nullptr, 0xffffffff);
    Context->OMSetDepthStencilState(mDepthStencilState.Get(), StencilReference);
}
