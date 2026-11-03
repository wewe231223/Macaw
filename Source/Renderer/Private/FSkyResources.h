/**
 * @file FSkyResources.h
 * @brief 등장방형 파노라마를 전체 화면 삼각형으로 그리는 기본 하늘 패스의 자원입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <span>

namespace Engine {
    struct FSceneView;
}

namespace Renderer {

    struct FPassTarget;

    /**
     * @brief 등장방형 파노라마를 전체 화면 삼각형으로 그리는 기본 하늘 패스의 자원입니다.
     */
    class FSkyResources {
    public:
        /**
         * @brief 기본 설정으로 FSkyResources 객체를 초기화합니다.
         */
        FSkyResources();
        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~FSkyResources();

    public:
        /**
         * @brief 상수 버퍼·샘플러·깊이·래스터라이저 상태를 생성하고 파노라마를 불러옵니다.
         * @param[in] Device GPU 자원을 생성할 Direct3D 11 장치입니다.
         * @param[in] PanoramaPath BC7 DDS 형식의 등장방형 파노라마 경로입니다.
         * @return 고정 GPU 상태를 생성하면 true입니다.
         * @note 파노라마를 읽지 못하면 경고만 보고하고 true를 반환하며 Render는 아무것도 그리지 않습니다.
         */
        bool Initialize(ID3D11Device* Device, const std::filesystem::path& PanoramaPath);
        /**
         * @brief 정점·픽셀 바이트 코드로 하늘 셰이더를 생성하고 둘 다 성공한 경우에만 교체합니다.
         * @param[in] Device GPU 자원을 생성할 Direct3D 11 장치입니다.
         * @param[in] VertexCode 전체 화면 삼각형 정점 셰이더의 컴파일된 바이트 코드입니다.
         * @param[in] PixelCode 하늘 픽셀 셰이더의 컴파일된 바이트 코드입니다.
         * @return 두 셰이더를 생성하면 true이며 실패하면 기존 셰이더를 유지하고 false입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);
        /**
         * @brief 불투명 패스의 깊이가 비어 있는 픽셀에만 하늘을 그립니다.
         * @param[in] Context GPU 명령을 기록할 Direct3D 11 컨텍스트입니다.
         * @param[in] Target 불투명 패스와 같은 색상·깊이 타깃입니다.
         * @param[in] View 시선 방향을 복원할 카메라 뷰입니다.
         * @return 제출한 드로 수이며 파노라마가 없거나 역행렬을 구하지 못하면 0입니다.
         */
        Core::Uint32 Render(ID3D11DeviceContext* Context, const Renderer::FPassTarget& Target, const Engine::FSceneView& View);

    private:
        /**
         * @brief BC7 DDS를 CPU 변환 없이 밉 체인 그대로 GPU 텍스처로 생성합니다.
         * @param[in] Device GPU 자원을 생성할 Direct3D 11 장치입니다.
         * @param[in] Path 읽을 DDS 파일 경로입니다.
         * @return 셰이더 리소스 뷰를 생성하면 true입니다.
         */
        bool LoadPanorama(ID3D11Device* Device, const std::filesystem::path& Path);

    private:
        /**
         * @brief 정점 버퍼 없이 원평면 전체 화면 삼각형을 생성하는 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};

        /**
         * @brief 시선 방향으로 파노라마를 샘플링하는 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};

        /**
         * @brief 이동을 제거한 역뷰투영 행렬을 픽셀 셰이더 b0에 전달하는 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};

        /**
         * @brief 가로 반복·세로 고정 주소 모드의 선형 샘플러입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};

        /**
         * @brief 원평면과 같거나 먼 픽셀만 통과시키고 깊이를 쓰지 않는 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthState{};

        /**
         * @brief 전체 화면 삼각형을 면 제거 없이 채우는 래스터라이저 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};

        /**
         * @brief 등장방형 파노라마의 셰이더 리소스 뷰이며 불러오지 못하면 비어 있습니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mPanorama{};
    };

}
