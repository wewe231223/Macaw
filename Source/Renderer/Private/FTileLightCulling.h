/**
 * @file FTileLightCulling.h
 * @brief 화면 타일별 광원 목록을 GPU에서 계산하고 렌더 단계에 연결합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"
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

    /**
     * @brief 광원 컬링용 역 뷰 투영 행렬과 화면·타일 크기입니다.
     */
    struct FTileCullingConstants {
        /**
         * @brief 화면 좌표를 월드로 복원할 역 뷰 투영 행렬입니다.
         */
        Core::FMatrix mInverseViewProjection{};

        /**
         * @brief 가로·세로 타일 수와 광원 수입니다.
         */
        std::array<Core::Uint32, 4> mDimensions{};

        /**
         * @brief 현재 렌더 뷰의 픽셀 너비·높이입니다.
         */
        Core::FVector4 mScreenSize{};
    };

    static_assert(sizeof(Renderer::FTileCullingConstants) == 96);

    /**
     * @brief 화면 타일별 광원 목록을 GPU에서 계산하고 렌더 단계에 연결합니다.
     */
    class FTileLightCulling {
    public:
        /**
         * @brief 타일 광원 컬링에 사용할 컴퓨트 셰이더를 생성합니다.
         * @param[in] Device GPU 자원을 생성할 D3D11 장치입니다.
         * @param[in] ComputeCode 컴파일된 컴퓨트 셰이더 바이트코드입니다.
         * @return 컴퓨트 셰이더 생성에 성공하면 true입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode);

        /**
         * @brief 16×16 화면 타일마다 영향을 주는 광원 목록을 GPU에서 계산합니다.
         * @param[in] Device GPU 자원을 생성할 D3D11 장치입니다.
         * @param[in] Context GPU 명령을 제출할 D3D11 장치 컨텍스트입니다.
         * @param[in] Lights 컬링할 GPU 광원 목록입니다.
         * @param[in] View 화면 크기와 뷰 투영 행렬입니다.
         * @param[in] LightCount 버퍼의 유효한 광원 수입니다.
         * @return 뷰·GPU 자원을 준비해 컬링을 제출했으면 true입니다.
         */
        bool Dispatch(ID3D11Device* Device, ID3D11DeviceContext* Context, const RenderCore::TStructuredBuffer<Renderer::FGpuLight>& Lights, const Engine::FSceneView& View, Core::Uint32 LightCount);

        /**
         * @brief 타일별 광원 범위·인덱스 버퍼를 연속된 셰이더 슬롯에 연결합니다.
         * @param[in] Context GPU 명령을 제출할 D3D11 장치 컨텍스트입니다.
         * @param[in] Stages 연결하거나 해제할 셰이더 단계입니다.
         * @param[in] FirstSlot 타일 범위 버퍼를 연결할 첫 SRV 슬롯입니다.
         */
        void Bind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t FirstSlot) const;

        /**
         * @brief 타일별 광원 범위·인덱스 버퍼의 셰이더 연결을 해제합니다.
         * @param[in] Context GPU 명령을 제출할 D3D11 장치 컨텍스트입니다.
         * @param[in] Stages 연결하거나 해제할 셰이더 단계입니다.
         * @param[in] FirstSlot 타일 범위 버퍼를 연결할 첫 SRV 슬롯입니다.
         */
        void Unbind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t FirstSlot) const;

    private:
        /**
         * @brief 타일과 광원의 교차를 검사하는 컴퓨트 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> mShader{};

        /**
         * @brief 현재 뷰·타일 크기를 전달하는 컴퓨트 상수 버퍼입니다.
         */
        RenderCore::TConstantBuffer<Renderer::FTileCullingConstants> mConstants{};

        /**
         * @brief 타일별 광원 목록의 시작·개수를 보관하는 GPU 버퍼입니다.
         */
        RenderCore::TRWStructuredBuffer<std::array<Core::Uint32, 2>> mTileRanges{};

        /**
         * @brief 타일에 영향을 주는 광원 인덱스를 이어 붙인 GPU 버퍼입니다.
         */
        RenderCore::TRWStructuredBuffer<Core::Uint32> mTileLightIndices{};

        /**
         * @brief 광원 인덱스 목록의 다음 기록 위치를 보관하는 GPU 카운터입니다.
         */
        RenderCore::TRWStructuredBuffer<Core::Uint32> mAllocator{};
    };

}
