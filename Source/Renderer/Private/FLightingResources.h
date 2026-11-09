/**
 * @file FLightingResources.h
 * @brief 타일별 광원 목록과 조명 상수를 GPU에 업로드하고 바인딩합니다.
 * @ingroup Renderer
 */

#pragma once

#include "FGpuLight.h"
#include "FTileLightCulling.h"
#include "Engine/FLightSceneProxy.h"
#include "RenderCore/TConstantBuffer.h"
#include "RenderCore/TStructuredBuffer.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief 카메라 위치와 광원·타일 개수를 셰이더 상수로 전달합니다.
     * @ingroup Renderer
     */
    struct FLightingConstants {
    public:
        /**
         * @brief 카메라 월드 위치 XYZ와 GPU 광원 개수를 저장합니다.
         */
        Core::FVector4 mCameraAndCount{};

        /**
         * @brief 타일 열·행 개수, 타일 한 변의 픽셀 수와 광원 개수·전체 순회 플래그를 저장합니다.
         */
        std::array<Core::Uint32, 4> mTileDimensions{};
    };

    /**
     * @brief 타일별 광원 목록과 조명 상수를 GPU에 업로드하고 바인딩합니다.
     * @ingroup Renderer
     */
    class FLightingResources {
    public:
        /**
         * @brief 타일 광원 컬링에 사용할 컴퓨트 셰이더를 생성합니다.
         * @param[in] Device GPU 자원을 생성할 D3D11 장치입니다.
         * @param[in] ComputeCode 컴파일된 컴퓨트 셰이더 바이트코드입니다.
         * @return 컴퓨트 셰이더 생성에 성공하면 true입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> ComputeCode);

        /**
         * @brief 현재 뷰의 타일 광원 목록과 카메라 상수를 GPU에 업로드합니다.
         * @param[in] Device GPU 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         * @param[in] Lights 렌더 장면에 등록된 원본 광원 프록시 목록입니다.
         * @param[in] View 카메라 행렬·위치와 출력 크기를 가진 뷰입니다.
         * @param[in] ShadowRanges 원본 광원 순서의 그림자 맵 시작 인덱스·개수이며 빈 범위면 그림자를 연결하지 않습니다.
         * @return 필요한 버퍼 생성과 업로드에 성공하면 true입니다.
         */
        bool Upload(ID3D11Device* Device, ID3D11DeviceContext* Context, std::span<const Engine::FLightSceneProxy> Lights, const Engine::FSceneView& View, std::span<const std::array<Core::Uint32, 2>> ShadowRanges = {});

        /**
         * @brief 타일 조명 버퍼와 상수의 셰이더 바인딩을 설정합니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         */
        void Bind(ID3D11DeviceContext* Context) const;

        /**
         * @brief 타일 조명 버퍼와 상수의 셰이더 바인딩을 해제합니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         */
        void Unbind(ID3D11DeviceContext* Context) const;

    private:
        /**
         * @brief 필요하면 구조화 버퍼를 확장하고 원소 데이터를 업로드합니다.
         * @param[in] Device 버퍼를 생성할 Direct3D 11 장치의 비소유 주소입니다.
         * @param[in] Context 버퍼를 갱신할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         * @return 필요한 버퍼 생성과 데이터 업로드에 성공하면 true입니다.
         */
        bool UploadLights(ID3D11Device* Device, ID3D11DeviceContext* Context);

    private:
        /**
         * @brief 현재 뷰에 사용할 GPU 광원 정보입니다.
         */
        Core::TArray<Renderer::FGpuLight> mLights{};

        /**
         * @brief 타일별 광원 목록을 계산하는 GPU 컬링 자원입니다.
         */
        Renderer::FTileLightCulling mCulling{};

        /**
         * @brief 광원 정보를 담는 GPU 구조화 버퍼입니다.
         */
        RenderCore::TStructuredBuffer<Renderer::FGpuLight> mLightBuffer{};

        /**
         * @brief 타일 목록 대신 전체 광원 목록을 사용할지 나타냅니다.
         */
        bool mUseGlobalLights{true};

        /**
         * @brief 카메라와 타일 격자 정보를 담는 상수 버퍼입니다.
         */
        RenderCore::TConstantBuffer<Renderer::FLightingConstants> mConstants{};
    };

}
