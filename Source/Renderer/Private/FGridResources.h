/**
 * @file FGridResources.h
 * @brief 에디터 그리드를 픽셀 셰이더로 그리는 패스의 자원입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <span>

namespace Engine {
    struct FSceneView;
}

namespace Renderer {

    struct FPassTarget;

    /**
     * @brief 에디터 그리드를 픽셀 셰이더로 그리는 패스의 자원입니다.
     */
    class FGridResources {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FGridResources);

    public:
        /**
         * @brief 기본 설정으로 FGridResources 객체를 초기화합니다.
         */
        FGridResources();
        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~FGridResources();

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FGridResources);

        /**
         * @brief 상수 버퍼·블렌드·깊이·래스터라이저 상태를 생성합니다.
         * @param[in] Device GPU 자원을 생성할 Direct3D 11 장치입니다.
         * @return 모든 고정 GPU 상태를 생성하면 true입니다.
         */
        bool Initialize(ID3D11Device* Device);
        /**
         * @brief 정점·픽셀 바이트 코드로 그리드 셰이더를 생성하고 둘 다 성공한 경우에만 교체합니다.
         * @param[in] Device GPU 자원을 생성할 Direct3D 11 장치입니다.
         * @param[in] VertexCode 평면 사각형 정점 셰이더의 컴파일된 바이트 코드입니다.
         * @param[in] PixelCode 그리드 픽셀 셰이더의 컴파일된 바이트 코드입니다.
         * @return 두 셰이더를 생성하면 true이며 실패하면 기존 셰이더를 유지하고 false입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);
        /**
         * @brief 불투명 깊이로 가려지는 그리드를 깊이 기록 없이 색상 타깃에 혼합합니다.
         * @param[in] Context GPU 명령을 기록할 Direct3D 11 컨텍스트입니다.
         * @param[in] Target 그릴 색상·깊이 타깃입니다.
         * @param[in] View 평면을 배치할 카메라 뷰입니다.
         * @param[in] Spacing 보조선 간격이며 주선은 10칸마다 그립니다.
         * @return 제출한 드로 수이며 셰이더가 없으면 0입니다.
         */
        Core::Uint32 Render(ID3D11DeviceContext* Context, const Renderer::FPassTarget& Target, const Engine::FSceneView& View, float Spacing);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 정점·픽셀 셰이더의 b0에 전달하는 그리드 상수입니다.
         */
        struct FConstants {
        public:
            /**
             * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
             */
            ReflectTypes(FConstants);

        public:
            /**
             * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
             * @return 등록된 타입 메타데이터의 비소유 주소입니다.
             */
            ReflectStruct(FConstants);

        private:
            /**
             * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
             */
            ReflectMembers();

        public:
            /**
             * @brief 월드 위치를 클립 공간으로 변환하는 행렬입니다.
             */
            Core::FMatrix mViewProjection{};
            ReflectProperty(mViewProjection, "ViewProjection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 카메라 월드 XY, 보조선 간격 Z와 평면 반지름 W입니다.
             */
            Core::FVector4 mCameraSpacingRadius{};
            ReflectProperty(mCameraSpacingRadius, "CameraSpacingRadius", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        };

    private:
        /**
         * @brief 정점 버퍼 없이 카메라 중심의 평면 사각형을 생성하는 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
        ReflectProperty(mVertexShader, "VertexShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 평면 좌표로 선 불투명도를 계산하는 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
        ReflectProperty(mPixelShader, "PixelShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 뷰투영 행렬과 카메라·간격·반지름을 b0에 전달하는 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
        ReflectProperty(mConstants, "Constants", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선 색상을 알파로 혼합하는 블렌드 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11BlendState> mBlend{};
        ReflectProperty(mBlend, "Blend", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 불투명 깊이를 검사하되 기록하지 않는 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthState{};
        ReflectProperty(mDepthState, "DepthState", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 평면의 양면을 모두 채우는 래스터라이저 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
        ReflectProperty(mRasterizer, "Rasterizer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
