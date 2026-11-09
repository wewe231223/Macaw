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
         * @brief 텍스처 UV와 장치 깊이를 월드 위치로 복원하는 행 우선 역행렬입니다.
         */
        Core::FMatrix mInverseViewProjection{};

        /**
         * @brief 복원된 월드 위치를 카메라 공간으로 변환하는 행 우선 뷰 행렬입니다.
         */
        Core::FMatrix mView{};

        /**
         * @brief 카메라 월드 위치 XYZ와 정사영 여부 W를 저장합니다.
         */
        Core::FVector4 mCameraPosition{};

        /**
         * @brief 렌더 타깃의 폭·높이 XY와 각각의 역수 ZW를 저장합니다.
         */
        Core::FVector4 mScreenSize{};

        /**
         * @brief 합성에 사용할 기본 산란 RGBA 색상입니다.
         */
        Core::FVector4 mFogColor{};

        /**
         * @brief 기준 밀도·높이 감쇠·안개 기준 Z·최대 불투명도를 순서대로 저장합니다.
         */
        Core::FVector4 mFogParameters{};

        /**
         * @brief 안개 시작 거리·적분 종료 거리·적용 제외 거리와 예약값을 저장합니다.
         */
        Core::FVector4 mFogDistances{};

        /**
         * @brief X에 복사 0·안개 1·깊이 2·FXAA 3 중 패스 종류를 저장하고 나머지는 예약합니다.
         */
        Core::FVector4 mSettings{};
    };

    static_assert(sizeof(Renderer::FPostProcessConstants) == 224);

}
