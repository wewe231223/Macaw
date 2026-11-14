/**
 * @file FTileLightCulling.cpp
 * @brief 화면 타일별 광원 목록을 GPU에서 계산하고 렌더 단계에 연결합니다.
 * @ingroup Renderer
 */

#include "Core/Types.h"
#include "FTileLightCulling.h"
#include "Core/Math/FVector4.h"

#include <algorithm>

namespace Renderer {

    /**
     * @brief 타일 광원 컬링에 사용할 컴퓨트 셰이더를 생성합니다.
     */
    bool FTileLightCulling::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode) {
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> Shader{};

        if (FAILED(Device->CreateComputeShader(ComputeCode.data(), ComputeCode.size(), nullptr, Shader.GetAddressOf()))) {
            return false;
        }

        mShader = std::move(Shader);

        return true;
    }

    /**
     * @brief 16×16 화면 타일마다 영향을 주는 광원 목록을 GPU에서 계산합니다.
     */
    bool FTileLightCulling::Dispatch(ID3D11Device* Device, ID3D11DeviceContext* Context, const RenderCore::TStructuredBuffer<Renderer::FGpuLight>& Lights, const Engine::FSceneView& View, Core::Uint32 LightCount) {
        const Core::Uint32 Columns{(static_cast<Core::Uint32>(std::max(View.mSize.x, 1.0f)) + 15) / 16};
        const Core::Uint32 Rows{(static_cast<Core::Uint32>(std::max(View.mSize.y, 1.0f)) + 15) / 16};
        const Core::Uint64 TileCount{static_cast<Core::Uint64>(Columns) * Rows};
        Core::FMatrix Inverse{};
        Core::FString Error{};

        if (mShader == nullptr || !View.mViewProjection.TryInverse(Inverse) || Columns > D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION || Rows > D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION || !mTileLightIndices.Reserve(Device, TileCount * LightCount, Error) || !mTileRanges.Reserve(Device, TileCount, Error) || !mAllocator.Reserve(Device, 1, Error) || !mConstants.Create(Device, Error)) {
            return false;
        }

        mConstants.Update(Context, {Inverse, {Columns, Rows, LightCount, 0}, {View.mSize.x, View.mSize.y, 0.0f, 0.0f}});

        const Core::Uint32 Zero{};

        if (!mAllocator.Upload(Context, std::span<const Core::Uint32>{&Zero, 1}, Error)) {
            return false;
        }

        Context->CSSetShader(mShader.Get(), nullptr, 0);
        Lights.Bind(Context, RenderCore::EShaderStage::Compute, 0);
        mTileRanges.BindOutput(Context, 0);
        mTileLightIndices.BindOutput(Context, 1);
        mAllocator.BindOutput(Context, 2);
        mConstants.Bind(Context, RenderCore::EShaderStage::Compute, 0);
        Context->Dispatch(Columns, Rows, 1);
        mTileRanges.UnbindOutput(Context, 0);
        mTileLightIndices.UnbindOutput(Context, 1);
        mAllocator.UnbindOutput(Context, 2);
        Lights.Unbind(Context, RenderCore::EShaderStage::Compute, 0);
        mConstants.Unbind(Context, RenderCore::EShaderStage::Compute, 0);
        Context->CSSetShader(nullptr, nullptr, 0);

        return true;
    }

    /**
     * @brief 타일별 광원 범위·인덱스 버퍼를 연속된 셰이더 슬롯에 연결합니다.
     */
    void FTileLightCulling::Bind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t FirstSlot) const {
        mTileRanges.Bind(Context, Stages, FirstSlot);
        mTileLightIndices.Bind(Context, Stages, FirstSlot + 1);
    }

    /**
     * @brief 타일별 광원 범위·인덱스 버퍼의 셰이더 연결을 해제합니다.
     */
    void FTileLightCulling::Unbind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t FirstSlot) const {
        mTileRanges.Unbind(Context, Stages, FirstSlot);
        mTileLightIndices.Unbind(Context, Stages, FirstSlot + 1);
    }

}
