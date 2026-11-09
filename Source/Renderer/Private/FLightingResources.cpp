/**
 * @file FLightingResources.cpp
 * @brief 타일별 광원 목록과 조명 상수를 GPU에 업로드하고 바인딩합니다.
 * @ingroup Renderer
 */

#include "FLightingResources.h"
#include "Core/Math.h"
#include "Core/Literals.h"
#include "Core/Stats.h"

#include <algorithm>

using namespace Core::Literals;

namespace Renderer {

    /**
     * @brief 타일 광원 컬링에 사용할 컴퓨트 셰이더를 생성합니다.
     */
    bool FLightingResources::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode) {
        return mCulling.CreateShaders(Device, ComputeCode);
    }

    /**
     * @brief 필요하면 구조화 버퍼를 확장하고 원소 데이터를 업로드합니다.
     */
    bool FLightingResources::UploadLights(ID3D11Device* Device, ID3D11DeviceContext* Context) {
        static_assert(sizeof(Renderer::FGpuLight) == 80);

        Core::FString Error{};

        // 광원이 없어도 셰이더가 읽을 뷰가 있어야 하므로 최소 한 칸을 확보합니다.
        if (!mLightBuffer.Reserve(Device, std::max<Core::Uint64>(mLights.size(), 1), Error)) {
            return false;
        }

        return mLights.empty() || mLightBuffer.Upload(Context, mLights, Error);
    }

    /**
     * @brief 현재 뷰의 타일 광원 목록과 카메라 상수를 GPU에 업로드합니다.
     */
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

    /**
     * @brief 타일 조명 버퍼와 상수의 셰이더 바인딩을 설정합니다.
     */
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

    /**
     * @brief 타일 조명 버퍼와 상수의 셰이더 바인딩을 해제합니다.
     */
    void FLightingResources::Unbind(ID3D11DeviceContext* Context) const {
        const RenderCore::EShaderStage Stages{RenderCore::EShaderStage::Vertex | RenderCore::EShaderStage::Pixel};

        mLightBuffer.Unbind(Context, Stages, 2);
        mCulling.Unbind(Context, Stages, 3);
    }

}
