/**
 * @file FLightingResources.h
 * @brief 타일별 광원 목록과 조명 상수를 GPU에 업로드하고 바인딩합니다.
 * @ingroup Renderer
 */

#pragma once

#include "FTiledLightGrid.h"

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
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FLightingConstants);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FLightingConstants);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 카메라 월드 위치 XYZ와 GPU 광원 개수를 저장합니다.
         */
        Core::FVector4 mCameraAndCount{};
        ReflectProperty(mCameraAndCount, "CameraAndCount", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 타일 열·행 개수, 타일 한 변의 픽셀 수와 예약값을 저장합니다.
         */
        std::array<Core::Uint32, 4> mTileDimensions{};
        ReflectProperty(mTileDimensions, "TileDimensions", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 타일별 광원 목록과 조명 상수를 GPU에 업로드하고 바인딩합니다.
     * @ingroup Renderer
     */
    class FLightingResources {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FLightingResources);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FLightingResources);

    public:
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
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 필요하면 구조화 버퍼를 확장하고 원소 데이터를 업로드합니다.
         * @param[in] Context 버퍼를 갱신할 Direct3D 11 장치 컨텍스트입니다.
         * @param[in] Context 명령을 기록할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         * @param[in] Index 갱신할 구조화 버퍼의 번호입니다.
         * @param[in] Data 업로드할 CPU 원소 데이터의 비소유 주소입니다.
         * @param[in] Count 업로드할 원소 개수입니다.
         * @param[in] Stride 구조화 버퍼 원소 하나의 바이트 수입니다.
         * @return 필요한 버퍼 생성과 데이터 업로드에 성공하면 true입니다.
         */
        bool UploadBuffer(ID3D11Device* Device, ID3D11DeviceContext* Context, Core::Uint32 Index, const void* Data, Core::Uint32 Count, Core::Uint32 Stride);

    private:
        /**
         * @brief 현재 뷰에 맞춰 구성한 CPU 타일 광원 목록입니다.
         */
        Renderer::FTiledLightGrid mGrid{};
        ReflectProperty(mGrid, "Grid", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광원·타일 구간·광원 인덱스를 담는 GPU 구조화 버퍼입니다.
         */
        std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, 3> mBuffers{};
        ReflectProperty(mBuffers, "Buffers", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 각 조명 구조화 버퍼의 셰이더 리소스 뷰입니다.
         */
        std::array<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>, 3> mViews{};
        ReflectProperty(mViews, "Views", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 각 조명 구조화 버퍼에 확보한 원소 수입니다.
         */
        std::array<Core::Uint32, 3> mCapacities{};
        ReflectProperty(mCapacities, "Capacities", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 카메라와 타일 격자 정보를 담는 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
        ReflectProperty(mConstants, "Constants", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
