/**
 * @file FSpriteRenderer.h
 * @brief 텍스처 스프라이트와 기본 애니메이션 아틀라스의 GPU 자원을 관리합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"
#include "Engine/UTexture2D.h"

#include <span>

namespace Engine {
    struct FPrimitiveSceneProxy;
    struct FSceneView;
}

namespace Renderer {

    /**
     * @brief 투명 스프라이트 쿼드와 기본 4×4 애니메이션 아틀라스의 GPU 자원을 관리합니다.
     */
    class FSpriteRenderer {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FSpriteRenderer);

    private:
        /**
         * @brief 스프라이트의 클립 행렬·UV 변환·색상 GPU 상수입니다.
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
             * @brief 로컬 쿼드에서 클립 공간으로의 행렬입니다.
             */
            Core::FMatrix mTransform{};
            ReflectProperty(mTransform, "Transform", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief XY 배율과 ZW 오프셋으로 구성된 UV 변환입니다.
             */
            Core::FVector4 mUVTransform{1.0f, 1.0f, 0.0f, 0.0f};
            ReflectProperty(mUVTransform, "UVTransform", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 표시할 RGBA 색상입니다.
             */
            Core::FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
            ReflectProperty(mColor, "Color", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        };

    public:
        /**
         * @brief 전달된 설정과 기본값으로 FSpriteRenderer 객체를 생성합니다.
         */
        FSpriteRenderer();

        /**
         * @brief 기본 소멸자이며, GPU 자원은 ComPtr 멤버가 해제합니다.
         */
        ~FSpriteRenderer();

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSpriteRenderer);

        /**
         * @brief 스프라이트 셰이더, 정점 버퍼와 상수 버퍼를 생성합니다.
         * @param[in] Device 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool Initialize(ID3D11Device* Device);

        /**
         * @brief 컴파일된 바이트코드로 정점·픽셀 셰이더를 생성하고 성공 시 교체합니다.
         * @param[in] Device 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] VertexCode 컴파일된 정점 셰이더 바이트코드입니다.
         * @param[in] PixelCode 컴파일된 픽셀 셰이더 바이트코드입니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);

        /**
         * @brief 쿼드 정렬과 UV 변환을 반영하여 텍스처 스프라이트를 그립니다.
         * @param[in] Context 그리기 명령을 기록할 Direct3D 11 장치 컨텍스트입니다.
         * @param[in] Proxy 이번 드로의 프리미티브 상태입니다.
         * @param[in] View 카메라 행렬과 뷰포트 크기입니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool Draw(ID3D11DeviceContext* Context, const Engine::FPrimitiveSceneProxy& Proxy, const Engine::FSceneView& View);

        /**
         * @brief 소유한 GPU 버퍼와 텍스처의 추정 바이트 수를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        std::size_t GetResourceBytes() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 스프라이트 변환·UV·색상 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
        ReflectProperty(mConstants, "Constants", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief GPU 정점 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
        ReflectProperty(mVertexShader, "VertexShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief GPU 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
        ReflectProperty(mPixelShader, "PixelShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스처 경계에서 클램프하는 샘플러입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
        ReflectProperty(mSampler, "Sampler", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기본 4×4 SubUV 아틀라스의 소유 객체입니다.
         */
        std::unique_ptr<Engine::UTexture2D> mDefaultAtlas{};
        ReflectProperty(mDefaultAtlas, "DefaultAtlas", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
