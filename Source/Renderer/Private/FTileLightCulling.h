#pragma once

#include "Core/ReflectionMacros.h"
#include "FGpuLight.h"
#include "Engine/FSceneView.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"
#include "RenderCore/TConstantBuffer.h"
#include "RenderCore/TRWStructuredBuffer.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <span>

namespace Renderer {

    struct FTileCullingConstants {
        Core::FMatrix mInverseViewProjection{};
        std::array<Core::Uint32, 4> mDimensions{};
        Core::FVector4 mScreenSize{};
    };

    static_assert(sizeof(Renderer::FTileCullingConstants) == 96);

    class FTileLightCulling {
    public:
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode);
        bool Dispatch(ID3D11Device* Device, ID3D11DeviceContext* Context, const RenderCore::TStructuredBuffer<Renderer::FGpuLight>& Lights, const Engine::FSceneView& View, Core::Uint32 LightCount);
        void Bind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 FirstSlot) const;
        void Unbind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 FirstSlot) const;

    private:
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> mShader{};
        RenderCore::TConstantBuffer<Renderer::FTileCullingConstants> mConstants{};
        RenderCore::TRWStructuredBuffer<std::array<Core::Uint32, 2>> mTileRanges{};
        RenderCore::TRWStructuredBuffer<Core::Uint32> mTileLightIndices{};
        RenderCore::TRWStructuredBuffer<Core::Uint32> mAllocator{};
    };

}
