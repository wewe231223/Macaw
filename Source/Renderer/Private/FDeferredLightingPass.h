/**
 * @file FDeferredLightingPass.h
 * @brief GBuffer를 읽어 조명 결과 또는 선택한 버퍼 시각화를 출력합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Math/FMatrix.h"
#include "RenderCore/TConstantBuffer.h"

#include <array>
#include <span>
#include <wrl/client.h>

namespace Engine {
    struct FSceneViewFamily;
}

namespace Renderer {

    class FDXRenderTarget;
    struct FPassTarget;

    /**
     * @brief GBuffer를 읽어 조명 결과 또는 선택한 버퍼 시각화를 출력합니다.
     * @ingroup Renderer
     */
    class FDeferredLightingPass {
    private:

        /**
         * @brief GBuffer 합성과 버퍼 시각화에 사용하는 CPU·셰이더 공유 상수 배치입니다.
         * @ingroup Renderer
         */
        struct FConstants {

            /**
             * @brief 깊이에서 월드 위치를 복원하는 뷰·투영 역행렬입니다.
             */
            Core::FMatrix mInverseViewProjection{};

            /**
             * @brief 출력 너비·높이의 역수입니다.
             */
            std::array<float, 2> mInverseSize{};

            /**
             * @brief 이 뷰어의 셰이딩·버퍼 시각화 모드입니다.
             */
            Core::Uint32 mViewMode{};

            /**
             * @brief CPU와 셰이더 상수 배치의 정렬을 맞추는 패딩입니다.
             */
            Core::Uint32 mPadding{};
        };

        static_assert(sizeof(FConstants) == 80);

    public:

        /**
         * @brief 지연 조명 합성에 필요한 상수 버퍼를 생성합니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         * @return GPU 상수 버퍼를 생성했으면 true입니다.
         */
        bool Initialize(ID3D11Device* Device);

        /**
         * @brief 컴파일된 바이트코드에서 패스의 정점·픽셀 셰이더를 만듭니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         * @param VertexCode 컴파일된 정점 셰이더 바이트코드입니다.
         * @param PixelCode 컴파일된 픽셀 셰이더 바이트코드입니다.
         * @return 두 셰이더를 생성했으면 true입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);

        /**
         * @brief GBuffer를 샘플링하여 조명 결과 또는 선택 버퍼를 출력 타깃에 그립니다.
         * @param Context 드로·복사 명령을 제출할 D3D11 장치 컨텍스트입니다.
         * @param Target 복사하거나 그릴 렌더 타깃입니다.
         * @param Output 렌더 결과를 기록할 출력 타깃입니다.
         * @param Family 장면 뷰·조명·표시 모드를 제공하는 뷰 패밀리입니다.
         * @param Rasterizer 호출자가 소유하는 래스터라이저 상태입니다.
         * @return 제출한 드로 호출 수입니다.
         */
        Core::Uint32 Render(ID3D11DeviceContext* Context, const Renderer::FDXRenderTarget& Target, const Renderer::FPassTarget& Output, const Engine::FSceneViewFamily& Family, ID3D11RasterizerState* Rasterizer);

    private:

        /**
         * @brief 패스별 셰이더 상수를 전송하는 GPU 버퍼입니다.
         */
        RenderCore::TConstantBuffer<FConstants> mConstants{};

        /**
         * @brief 패스의 정점 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};

        /**
         * @brief 패스의 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    };

}
