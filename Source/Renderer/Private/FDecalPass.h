/**
 * @file FDecalPass.h
 * @brief 깊이로 복원한 표면에 데칼을 투영하고 GBuffer의 색상·노멀을 갱신합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"
#include "RenderCore/TConstantBuffer.h"

#include <array>
#include <d3d11.h>
#include <span>
#include <wrl/client.h>

namespace Engine {
    struct FDecalSceneProxy;
    struct FSceneViewFamily;
}

namespace Renderer {

    class FDXRenderTarget;

    /**
     * @brief 깊이로 복원한 표면에 데칼을 투영하고 GBuffer의 색상·노멀을 갱신합니다.
     * @ingroup Renderer
     */
    class FDecalPass {
    private:

        /**
         * @brief 깊이 복원과 데칼 투영에 사용하는 CPU·셰이더 공유 상수 배치입니다.
         * @ingroup Renderer
         */
        struct FConstants {

            /**
             * @brief 깊이에서 월드 위치를 복원하는 뷰·투영 역행렬입니다.
             */
            Core::FMatrix mInverseViewProjection{};

            /**
             * @brief 월드 좌표를 데칼 투영 박스 좌표로 변환하는 역행렬입니다.
             */
            Core::FMatrix mWorldToDecal{};

            /**
             * @brief 화면 너비·높이의 역수, 불투명도, 노멀 강도 순서의 셰이더 상수입니다.
             */
            Core::FVector4 mScreenAndOpacity{};

            /**
             * @brief 색상 텍스처 존재·색상 출력 활성화 플래그와 예약 값입니다.
             */
            std::array<Core::Uint32, 4> mTextureFlags{};
        };

        static_assert(sizeof(FConstants) == 160);

    public:

        /**
         * @brief 데칼 투영의 상수 버퍼와 블렌드·샘플러·깊이 상태를 만듭니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         * @param ColorBlend 색상 출력에 사용할 블렌드 설정입니다.
         * @return 필요한 GPU 상태를 모두 생성했으면 true입니다.
         */
        bool Initialize(ID3D11Device* Device, const D3D11_BLEND_DESC& ColorBlend);

        /**
         * @brief 컴파일된 바이트코드에서 패스의 정점·픽셀 셰이더를 만듭니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         * @param VertexCode 컴파일된 정점 셰이더 바이트코드입니다.
         * @param PixelCode 컴파일된 픽셀 셰이더 바이트코드입니다.
         * @return 두 셰이더를 생성했으면 true입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);

        /**
         * @brief 데칼 수신 표면에 정렬된 데칼을 투영하여 GBuffer를 갱신합니다.
         * @param Device 리소스를 생성할 D3D11 장치입니다.
         * @param Context 드로·복사 명령을 제출할 D3D11 장치 컨텍스트입니다.
         * @param Target 복사하거나 그릴 렌더 타깃입니다.
         * @param Family 장면 뷰·조명·표시 모드를 제공하는 뷰 패밀리입니다.
         * @param Decals 장면에 등록된 데칼 프록시 목록입니다.
         * @return 실제로 제출한 드로 호출 수입니다.
         */
        Core::Uint32 Render(ID3D11Device* Device, ID3D11DeviceContext* Context, Renderer::FDXRenderTarget& Target, const Engine::FSceneViewFamily& Family, std::span<const Engine::FDecalSceneProxy> Decals);

    private:

        /**
         * @brief 투영한 데칼 박스가 차지하는 화면 픽셀 사각형을 계산합니다.
         * @param DecalToClip 정규화된 데칼 박스를 클립 공간으로 옮기는 행렬입니다.
         * @param Width 출력 또는 UI 영역의 너비(픽셀)입니다.
         * @param Height 출력 또는 UI 영역의 높이(픽셀)입니다.
         * @return 출력 크기로 제한한 시저 사각형입니다.
         */
        static D3D11_RECT GetScissor(const Core::FMatrix& DecalToClip, Core::Uint32 Width, Core::Uint32 Height);

    private:

        /**
         * @brief 패스별 셰이더 상수를 전송하는 GPU 버퍼입니다.
         */
        RenderCore::TConstantBuffer<FConstants> mConstants{};

        /**
         * @brief 데칼 수신 표면만 통과시키는 깊이·스텐실 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mTestReceiver{};

        /**
         * @brief 패스에 사용하는 래스터라이저 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};

        /**
         * @brief 텍스처 샘플링 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};

        /**
         * @brief 데칼 색상 혼합에 사용하는 GPU 블렌드 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11BlendState> mBlend{};

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
