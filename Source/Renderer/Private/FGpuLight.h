/**
 * @file FGpuLight.h
 * @brief 위치·방향·색상·감쇠·그림자를 GPU에 전달하는 float4 묶음입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector4.h"

namespace Renderer {

    /**
     * @brief 위치·방향·색상·감쇠·그림자를 GPU에 전달하는 float4 묶음입니다.
     */
    struct FGpuLight {
    public:
        /**
         * @brief XYZ 광원 위치와 W 영향 반경입니다.
         */
        Core::FVector4 mPositionAndRadius{};

        /**
         * @brief XYZ 광원 방향과 W 종류입니다.
         */
        Core::FVector4 mDirectionAndType{};

        /**
         * @brief RGB 광원 색상과 W 세기입니다.
         */
        Core::FVector4 mColorAndIntensity{};

        /**
         * @brief 거리 감쇠와 스포트라이트 원뿔 계산 값입니다.
         */
        Core::FVector4 mFalloffAndCones{};

        /**
         * @brief 광원에 대응하는 그림자 인덱스·설정입니다.
         */
        Core::FVector4 mShadowInfo{};
    };

}
