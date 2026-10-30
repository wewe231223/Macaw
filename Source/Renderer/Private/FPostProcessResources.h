/**
 * @file FPostProcessResources.h
 * @brief 전체 화면 삼각형으로 안개를 합성하고 깊이 시각화 또는 FXAA를 최종 컬러에 출력합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <span>

namespace Engine {
    class UTexture2D;
    struct FSceneViewFamily;
    struct FHeightFogSceneProxy;
}

namespace Renderer {

    class FDXRenderTarget;

    /**
     * @brief 전체 화면 삼각형으로 안개를 합성하고 깊이 시각화 또는 FXAA를 최종 컬러에 출력합니다.
     */
    class FPostProcessResources {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FPostProcessResources);

    public:
        /**
         * @brief 기본 설정으로 FPostProcessResources 객체를 초기화합니다.
         */
        FPostProcessResources();
        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~FPostProcessResources();

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FPostProcessResources);

        /**
         * @brief 상수 버퍼·선형 Clamp 샘플러·깊이 비활성 상태와 면 제거 없는 래스터라이저를 생성합니다.
         * @param[in] Device GPU 자원을 생성하거나 조회할 Direct3D 11 장치입니다.
         * @return 모든 GPU 자원을 생성하면 true이며 하나라도 생성에 실패하면 false입니다.
         */
        bool Initialize(ID3D11Device* Device);
        /**
         * @brief 정점·픽셀 바이트 코드로 후처리 셰이더를 생성하고 둘 다 성공한 경우에만 교체합니다.
         * @param[in] Device GPU 자원을 생성하거나 조회할 Direct3D 11 장치입니다.
         * @param[in] VertexCode 전체 화면 삼각형 정점 셰이더의 컴파일된 바이트 코드입니다.
         * @param[in] PixelCode 후처리 픽셀 셰이더의 컴파일된 바이트 코드입니다.
         * @return 두 셰이더를 생성하면 true이며 실패하면 기존 셰이더를 유지하고 false입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);
        /**
         * @brief 가시 안개를 교대 타깃에 합성한 뒤 복사·깊이 시각화·FXAA 중 최종 패스를 실행합니다.
         * @param[in] Context GPU 명령을 기록하고 읽기·쓰기 상태를 설정할 Direct3D 11 컨텍스트입니다.
         * @param[in,out] Target 입력 컬러·깊이와 교대 컬러·최종 출력을 소유한 렌더 타깃입니다.
         * @param[in] Family 카메라·버퍼 모드·ShowFlag를 포함한 현재 뷰의 렌더 요청입니다.
         * @param[in] Fogs 순서대로 합성할 장면의 높이 안개 프록시 목록입니다.
         * @return 제출한 전체 화면 드로 수이며 역행렬을 구하지 못해 원본을 복사하면 0입니다.
         * @note Normal·Depth에는 안개와 FXAA를 적용하지 않습니다. 뷰포트 크기는 호출자가 미리 설정해야 합니다.
         */
        Core::Uint32 Render(ID3D11DeviceContext* Context, Renderer::FDXRenderTarget& Target, const Engine::FSceneViewFamily& Family, std::span<const Engine::FHeightFogSceneProxy> Fogs);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
        /**
         * @brief 입력 컬러·깊이를 읽어 다른 컬러 타깃에 전체 화면 삼각형을 그리고 SRV·RTV 바인딩을 해제합니다.
         * @param[in] Context GPU 명령을 기록하고 읽기·쓰기 상태를 설정할 Direct3D 11 컨텍스트입니다.
         * @param[in] Source 샘플링할 입력 컬러 텍스처이며 Destination과 다른 자원이어야 합니다.
         * @param[in] Destination 후처리 결과를 기록할 컬러 텍스처이며 Source와 다른 자원이어야 합니다.
         * @param[in] Depth ShaderResourceView로 읽을 장면 깊이 텍스처입니다.
         */
        void Draw(ID3D11DeviceContext* Context, Engine::UTexture2D* Source, Engine::UTexture2D* Destination, Engine::UTexture2D* Depth);

    private:
        /**
         * @brief 정점 버퍼 없이 정점 ID로 전체 화면 삼각형을 생성하는 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
        ReflectProperty(mVertexShader, "VertexShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 복사·안개·Scene Depth·FXAA 분기를 수행하는 후처리 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
        ReflectProperty(mPixelShader, "PixelShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief FPostProcessConstants를 픽셀 셰이더 b0에 전달하는 GPU 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
        ReflectProperty(mConstants, "Constants", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief SceneColor를 선형 필터와 가장자리 고정 주소 모드로 읽는 샘플러입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
        ReflectProperty(mSampler, "Sampler", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 전체 화면 후처리에서 깊이 검사와 깊이 쓰기를 비활성화하는 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthDisabled{};
        ReflectProperty(mDepthDisabled, "DepthDisabled", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 전체 화면 삼각형을 면 제거 없이 채우는 래스터라이저 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
        ReflectProperty(mRasterizer, "Rasterizer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
