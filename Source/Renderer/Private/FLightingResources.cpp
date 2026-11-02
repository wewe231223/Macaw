#include "FLightingResources.h"
#include "Core/Math.h"
#include "Core/Literals.h"
#include "Core/Stats.h"

#include <algorithm>
#include <bit>
#include <limits>

using namespace Core::Literals;

namespace Renderer {

    bool FLightingResources::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode) {
        return mCulling.CreateShaders(Device, ComputeCode);
    }

    bool FLightingResources::UploadLights(ID3D11Device* Device, ID3D11DeviceContext* Context) {
        constexpr Core::Uint32 Stride{sizeof(Renderer::FGpuLight)};
        static_assert(Stride == 80);
        constexpr Core::Uint32 Maximum{std::numeric_limits<UINT>::max() / Stride};

        if (mLights.size() > Maximum) {
            return false;
        }

        const Core::Uint32 Count{static_cast<Core::Uint32>(mLights.size())};
        const Core::Uint32 Required{std::max(Count, 1u)};

        if (mLightCapacity < Required) {
            const Core::Uint32 Capacity{std::min(std::bit_ceil(Required), Maximum)};
            D3D11_BUFFER_DESC Description{};
            Description.ByteWidth = Capacity * Stride;
            Description.Usage = D3D11_USAGE_DEFAULT;
            Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            Description.StructureByteStride = Stride;
            Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer{};
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Resource{};

            if (FAILED(Device->CreateBuffer(&Description, nullptr, Buffer.GetAddressOf())) || FAILED(Device->CreateShaderResourceView(Buffer.Get(), nullptr, Resource.GetAddressOf()))) {
                return false;
            }

            mLightBuffer = std::move(Buffer);
            mLightView = std::move(Resource);
            mLightCapacity = Capacity;
        }

        if (Count > 0) {
            const D3D11_BOX Box{0, 0, 0, Count * Stride, 1, 1};
            Context->UpdateSubresource(mLightBuffer.Get(), 0, &Box, mLights.data(), 0, 0);
        }

        return true;
    }

    bool FLightingResources::Upload(ID3D11Device* Device, ID3D11DeviceContext* Context, std::span<const Engine::FLightSceneProxy> Lights, const Engine::FSceneView& View, std::span<const std::array<Core::Uint32, 2>> ShadowRanges) {
        static Core::FStat PrepareTime{"render", "GPU light culling submission CPU", Core::EStatUnit::Milliseconds};
        static Core::FStat SubmittedLights{"render", "Input lights (sum of views)"};
        const Core::FScopedStatTimer StatTimer{PrepareTime};
        SubmittedLights.Add(static_cast<double>(Lights.size()));
        Unbind(Context);
        mLights.clear();
        mLights.reserve(Lights.size());

        for (std::size_t Index{}; Index < Lights.size(); ++Index) {
            const Engine::FLightSceneProxy& Light{Lights[Index]};

            if (!Light.mVisible || Light.mIntensity <= 0.0f) {
                continue;
            }

            mLights.push_back({{Light.mPosition, Light.mAttenuationRadius}, {Light.mDirection, static_cast<float>(Light.mType)}, {Light.mColor.x, Light.mColor.y, Light.mColor.z, Light.mIntensity}, {Light.mLightFalloffExponent, Core::Math::Cos(Light.mInnerConeAngle * 1_deg), Core::Math::Cos(Light.mOuterConeAngle * 1_deg), 0.0f}});

            if (Index < ShadowRanges.size()) {
                mLights.back().mShadowInfo = {static_cast<float>(ShadowRanges[Index][0]), static_cast<float>(ShadowRanges[Index][1]), 0.0f, 0.0f};
            }
        }

        if (!UploadLights(Device, Context)) {
            return false;
        }

        const Core::Uint32 Count{static_cast<Core::Uint32>(mLights.size())};
        const Core::Uint32 Columns{(static_cast<Core::Uint32>(std::max(View.mSize.x, 1.0f)) + 15) / 16};
        const Core::Uint32 Rows{(static_cast<Core::Uint32>(std::max(View.mSize.y, 1.0f)) + 15) / 16};
        mUseGlobalLights = Count == 0 || !mCulling.Dispatch(Device, Context, mLightView.Get(), View, Count);
        const Renderer::FLightingConstants Constants{{View.mLocation, static_cast<float>(Count)}, {Columns, Rows, 16, Count | (mUseGlobalLights ? 0x80000000u : 0u)}};

        if (mConstants == nullptr) {
            D3D11_BUFFER_DESC Description{};
            Description.ByteWidth = sizeof(Renderer::FLightingConstants);
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
        const std::array<ID3D11ShaderResourceView*, 2> Tiles{mCulling.GetViews()};
        const std::array<ID3D11ShaderResourceView*, 3> Views{mLightView.Get(), mUseGlobalLights ? nullptr : Tiles[0], mUseGlobalLights ? nullptr : Tiles[1]};
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

}
