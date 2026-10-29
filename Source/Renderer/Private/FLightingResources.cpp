#include "FLightingResources.h"

#include <algorithm>
#include <limits>

bool FLightingResources::UploadBuffer(ID3D11Device* Device, ID3D11DeviceContext* Context, Uint32 Index, const void* Data, Uint32 Count, Uint32 Stride) {
    if (Count > std::numeric_limits<Uint32>::max() / Stride) {
        return false;
    }
    const Uint32 Required{std::max(Count, 1u)};
    if (mCapacities[Index] < Required) {
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = Required * Stride;
        Description.Usage = D3D11_USAGE_DEFAULT;
        Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        Description.StructureByteStride = Stride;
        Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Resource{};
        if (FAILED(Device->CreateBuffer(&Description, nullptr, Buffer.GetAddressOf())) || FAILED(Device->CreateShaderResourceView(Buffer.Get(), nullptr, Resource.GetAddressOf()))) {
            return false;
        }
        mBuffers[Index] = std::move(Buffer);
        mViews[Index] = std::move(Resource);
        mCapacities[Index] = Required;
    }
    if (Count > 0) {
        const D3D11_BOX Box{0, 0, 0, Count * Stride, 1, 1};
        Context->UpdateSubresource(mBuffers[Index].Get(), 0, &Box, Data, 0, 0);
    }
    return true;
}

bool FLightingResources::Upload(ID3D11Device* Device, ID3D11DeviceContext* Context, std::span<const FLightSceneProxy> Lights, const FSceneView& View) {
    Unbind(Context);
    mGrid.Build(Lights, View);
    if (!UploadBuffer(Device, Context, 0, mGrid.GetLights().data(), static_cast<Uint32>(mGrid.GetLights().size()), sizeof(FGpuLight)) || !UploadBuffer(Device, Context, 1, mGrid.GetRanges().data(), static_cast<Uint32>(mGrid.GetRanges().size()), sizeof(std::array<Uint32, 2>)) || !UploadBuffer(Device, Context, 2, mGrid.GetIndices().data(), static_cast<Uint32>(mGrid.GetIndices().size()), sizeof(Uint32))) {
        return false;
    }
    const FLightingConstants Constants{{View.mLocation, static_cast<float>(mGrid.GetLights().size())}, {mGrid.GetColumns(), mGrid.GetRows(), 16, 0}};
    if (mConstants == nullptr) {
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = sizeof(FLightingConstants);
        Description.Usage = D3D11_USAGE_DEFAULT;
        Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(Device->CreateBuffer(&Description, nullptr, mConstants.GetAddressOf()))) {
            return false;
        }
    }
    Context->UpdateSubresource(mConstants.Get(), 0, nullptr, &Constants, 0, 0);
    Bind(Context);
    return true;
}

void FLightingResources::Bind(ID3D11DeviceContext* Context) const {
    const std::array<ID3D11ShaderResourceView*, 3> Views{mViews[0].Get(), mViews[1].Get(), mViews[2].Get()};
    ID3D11Buffer* Constants{mConstants.Get()};
    Context->VSSetShaderResources(2, static_cast<UINT>(Views.size()), Views.data());
    Context->PSSetShaderResources(2, static_cast<UINT>(Views.size()), Views.data());
    Context->VSSetConstantBuffers(3, 1, &Constants);
    Context->PSSetConstantBuffers(3, 1, &Constants);
}

void FLightingResources::Unbind(ID3D11DeviceContext* Context) const {
    const std::array<ID3D11ShaderResourceView*, 3> Empty{};
    Context->VSSetShaderResources(2, static_cast<UINT>(Empty.size()), Empty.data());
    Context->PSSetShaderResources(2, static_cast<UINT>(Empty.size()), Empty.data());
}
