#include "FTileLightCulling.h"
#include "Core/Math/FVector4.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace Renderer {

    bool FTileLightCulling::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode) {
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> Shader{};

        if (FAILED(Device->CreateComputeShader(ComputeCode.data(), ComputeCode.size(), nullptr, Shader.GetAddressOf()))) {
            return false;
        }

        mShader = std::move(Shader);
        return true;
    }

    bool FTileLightCulling::EnsureBuffer(ID3D11Device* Device, Core::Uint32 Index, Core::Uint64 Count, Core::Uint32 Stride) {
        const Core::Uint64 Maximum{std::min<Core::Uint64>(std::numeric_limits<UINT>::max() / Stride, Core::Uint64{1} << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP)};

        if (Count > Maximum) {
            return false;
        }

        const Core::Uint32 Required{static_cast<Core::Uint32>(std::max<Core::Uint64>(Count, 1))};

        if (mCapacities[Index] >= Required) {
            return true;
        }

        const Core::Uint32 Capacity{static_cast<Core::Uint32>(std::min<Core::Uint64>(std::bit_ceil(Required), Maximum))};
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = Capacity * Stride;
        Description.Usage = D3D11_USAGE_DEFAULT;
        Description.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        Description.StructureByteStride = Stride;
        Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View{};
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> Output{};

        if (FAILED(Device->CreateBuffer(&Description, nullptr, Buffer.GetAddressOf())) || FAILED(Device->CreateShaderResourceView(Buffer.Get(), nullptr, View.GetAddressOf())) || FAILED(Device->CreateUnorderedAccessView(Buffer.Get(), nullptr, Output.GetAddressOf()))) {
            return false;
        }

        mBuffers[Index] = std::move(Buffer);
        mViews[Index] = std::move(View);
        mOutputs[Index] = std::move(Output);
        mCapacities[Index] = Capacity;
        return true;
    }

    bool FTileLightCulling::Dispatch(ID3D11Device* Device, ID3D11DeviceContext* Context, ID3D11ShaderResourceView* Lights, const Engine::FSceneView& View, Core::Uint32 LightCount) {
        const Core::Uint32 Columns{(static_cast<Core::Uint32>(std::max(View.mSize.x, 1.0f)) + 15) / 16};
        const Core::Uint32 Rows{(static_cast<Core::Uint32>(std::max(View.mSize.y, 1.0f)) + 15) / 16};
        const Core::Uint64 TileCount{static_cast<Core::Uint64>(Columns) * Rows};
        Core::FMatrix Inverse{};

        if (mShader == nullptr || !View.mViewProjection.TryInverse(Inverse) || Columns > D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION || Rows > D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION || !EnsureBuffer(Device, 1, TileCount * LightCount, sizeof(Core::Uint32)) || !EnsureBuffer(Device, 0, TileCount, sizeof(Core::Uint32) * 2) || !EnsureBuffer(Device, 2, 1, sizeof(Core::Uint32))) {
            return false;
        }

        if (mConstants == nullptr) {
            D3D11_BUFFER_DESC Description{};
            Description.ByteWidth = sizeof(Core::FMatrix) + sizeof(Core::FVector4) * 2;
            Description.Usage = D3D11_USAGE_DEFAULT;
            Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            if (FAILED(Device->CreateBuffer(&Description, nullptr, mConstants.GetAddressOf()))) {
                return false;
            }
        }

        static_assert(sizeof(Core::FMatrix) == 64);
        static_assert(sizeof(Core::FVector4) == 16);
        std::array<Core::Uint8, 96> Constants{};
        const std::array<Core::Uint32, 4> Dimensions{Columns, Rows, LightCount, 0};
        const Core::FVector4 Screen{View.mSize.x, View.mSize.y, 0.0f, 0.0f};
        std::memcpy(Constants.data(), &Inverse, sizeof(Inverse));
        std::memcpy(Constants.data() + sizeof(Inverse), Dimensions.data(), sizeof(Dimensions));
        std::memcpy(Constants.data() + sizeof(Inverse) + sizeof(Dimensions), &Screen, sizeof(Screen));
        Context->UpdateSubresource(mConstants.Get(), 0, nullptr, Constants.data(), 0, 0);
        const Core::Uint32 Zero{};
        Context->UpdateSubresource(mBuffers[2].Get(), 0, nullptr, &Zero, 0, 0);
        Context->CSSetShader(mShader.Get(), nullptr, 0);
        Context->CSSetShaderResources(0, 1, &Lights);
        const std::array<ID3D11UnorderedAccessView*, 3> Outputs{mOutputs[0].Get(), mOutputs[1].Get(), mOutputs[2].Get()};
        Context->CSSetUnorderedAccessViews(0, static_cast<UINT>(Outputs.size()), Outputs.data(), nullptr);
        ID3D11Buffer* Buffer{mConstants.Get()};
        Context->CSSetConstantBuffers(0, 1, &Buffer);
        Context->Dispatch(Columns, Rows, 1);
        const std::array<ID3D11UnorderedAccessView*, 3> EmptyOutputs{};
        Context->CSSetUnorderedAccessViews(0, static_cast<UINT>(EmptyOutputs.size()), EmptyOutputs.data(), nullptr);
        ID3D11ShaderResourceView* EmptyView{};
        Context->CSSetShaderResources(0, 1, &EmptyView);
        ID3D11Buffer* EmptyBuffer{};
        Context->CSSetConstantBuffers(0, 1, &EmptyBuffer);
        Context->CSSetShader(nullptr, nullptr, 0);
        return true;
    }

    std::array<ID3D11ShaderResourceView*, 2> FTileLightCulling::GetViews() const {
        return {mViews[0].Get(), mViews[1].Get()};
    }

}
