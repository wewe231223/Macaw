#include "FLightingResources.h"
#include "Core/Math.h"
#include "Core/Literals.h"
#include "Core/Stats.h"

#include <algorithm>

using namespace Core::Literals;

namespace Renderer {

    bool FLightingResources::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode) {
        return mCulling.CreateShaders(Device, ComputeCode);
    }

    bool FLightingResources::UploadLights(ID3D11Device* Device, ID3D11DeviceContext* Context) {
        static_assert(sizeof(Renderer::FGpuLight) == 80);
        Core::FString Error{};

        // 광원이 없어도 셰이더가 읽을 뷰가 있어야 하므로 최소 한 칸을 확보합니다.
        if (!mLightBuffer.Reserve(Device, std::max<Core::Uint64>(mLights.size(), 1), Error)) {
            return false;
        }

        return mLights.empty() || mLightBuffer.Upload(Context, mLights, Error);
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
        mUseGlobalLights = Count == 0 || !mCulling.Dispatch(Device, Context, mLightBuffer, View, Count);
        const Renderer::FLightingConstants Constants{{View.mLocation, static_cast<float>(Count)}, {Columns, Rows, 16, Count | (mUseGlobalLights ? 0x80000000u : 0u)}};

        Core::FString Error{};

        if (!mConstants.Create(Device, Error)) {
            return false;
        }

        mConstants.Update(Context, Constants);
        Bind(Context);
        return true;
    }

    void FLightingResources::Bind(ID3D11DeviceContext* Context) const {
        const RenderCore::EShaderStage Stages{RenderCore::EShaderStage::Vertex | RenderCore::EShaderStage::Pixel};
        mLightBuffer.Bind(Context, Stages, 2);

        if (mUseGlobalLights) {
            mCulling.Unbind(Context, Stages, 3);
        } else {
            mCulling.Bind(Context, Stages, 3);
        }

        mConstants.Bind(Context, Stages, 3);
    }

    void FLightingResources::Unbind(ID3D11DeviceContext* Context) const {
        const RenderCore::EShaderStage Stages{RenderCore::EShaderStage::Vertex | RenderCore::EShaderStage::Pixel};
        mLightBuffer.Unbind(Context, Stages, 2);
        mCulling.Unbind(Context, Stages, 3);
    }

}
