/**
 * @file FPostProcessConstants.h
 * @brief 월드 위치 복원·높이 안개·깊이 시각화·FXAA에 사용하는 224바이트 GPU 상수입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"

namespace Renderer {

    /**
     * @brief 월드 위치 복원·높이 안개·깊이 시각화·FXAA에 사용하는 224바이트 GPU 상수입니다.
     */
    struct alignas(16) FPostProcessConstants {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FPostProcessConstants);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FPostProcessConstants);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 텍스처 UV와 장치 깊이를 월드 위치로 복원하는 행 우선 역행렬입니다.
         */
        Core::FMatrix mInverseViewProjection{};
        ReflectProperty(mInverseViewProjection, "InverseViewProjection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 복원된 월드 위치를 카메라 공간으로 변환하는 행 우선 뷰 행렬입니다.
         */
        Core::FMatrix mView{};
        ReflectProperty(mView, "View", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 카메라 월드 위치 XYZ와 정사영 여부 W를 저장합니다.
         */
        Core::FVector4 mCameraPosition{};
        ReflectProperty(mCameraPosition, "CameraPosition", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 렌더 타깃의 폭·높이 XY와 각각의 역수 ZW를 저장합니다.
         */
        Core::FVector4 mScreenSize{};
        ReflectProperty(mScreenSize, "ScreenSize", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 합성에 사용할 기본 산란 RGBA 색상입니다.
         */
        Core::FVector4 mFogColor{};
        ReflectProperty(mFogColor, "FogColor", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기준 밀도·높이 감쇠·안개 기준 Z·최대 불투명도를 순서대로 저장합니다.
         */
        Core::FVector4 mFogParameters{};
        ReflectProperty(mFogParameters, "FogParameters", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 안개 시작 거리·적분 종료 거리·적용 제외 거리와 예약값을 저장합니다.
         */
        Core::FVector4 mFogDistances{};
        ReflectProperty(mFogDistances, "FogDistances", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief X에 복사 0·안개 1·깊이 2·FXAA 3 중 패스 종류를 저장하고 나머지는 예약합니다.
         */
        Core::FVector4 mSettings{};
        ReflectProperty(mSettings, "Settings", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    static_assert(sizeof(Renderer::FPostProcessConstants) == 224);

}
