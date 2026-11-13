/**
 * @file EViewMode.h
 * @brief 장면 전체의 채우기 방식을 선택합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EViewMode.generated.h"

namespace Engine {

    /**
     * @brief 장면 전체에 적용하는 메시 채우기 방식입니다.
     */
    UENUM()
    enum class EViewMode : Core::Uint8 {

        /**
         * @brief 픽셀별 확산·정반사 조명 모드이며 Lit과 같은 값입니다.
         */
        Lit_Phong = 0,

        /**
         * @brief 정점에서 계산한 조명을 보간하는 Gouraud 모드입니다.
         */
        Lit_Gouraud = 2,

        /**
         * @brief 픽셀별 Lambert 확산 조명 모드입니다.
         */
        Lit_Lambert = 3,

        /**
         * @brief 광원 계산 없이 재질 기본색을 표시합니다.
         */
        Unlit = 4,

        /**
         * @brief 월드 공간 법선을 RGB 색상으로 표시합니다.
         */
        WorldNormal = 5,

        /**
         * @brief 장면 깊이를 뷰 공간 거리로 복원하여 회색조로 시각화합니다.
         */
        SceneDepth = 6,

        /**
         * @brief 삼각형의 변을 와이어프레임으로 표시합니다.
         */
        Wireframe = 1,

        /**
         * @brief GBuffer의 기본 색상을 표시합니다.
         */
        BaseColor = 7,

        /**
         * @brief GBuffer의 반사광 색상을 표시합니다.
         */
        Specular = 8,

        /**
         * @brief GBuffer의 발광 색상을 표시합니다.
         */
        Emissive = 9,

        /**
         * @brief GBuffer의 환경광 색상을 표시합니다.
         */
        AmbientColor = 10,

        /**
         * @brief GBuffer의 반사 파라미터를 표시합니다.
         */
        Reflection = 11,

        /**
         * @brief 기본 색상 텍스처의 알파를 표시합니다.
         */
        BaseColorAlpha = 12,

        /**
         * @brief 표면의 광택 지수를 표시합니다.
         */
        Shininess = 13,

        /**
         * @brief 표면의 그림자 수신 여부를 표시합니다.
         */
        ReceiveShadows = 14,

        /**
         * @brief 장면 스텐실의 표면 분류를 표시합니다.
         */
        SceneStencil = 16,

        /**
         * @brief 재질과 기본 조명을 사용하여 면을 채웁니다.
         */
        Lit = Lit_Phong UMETA(Hidden)
    };

    bool IsBufferVisualization(Engine::EViewMode Mode);

}
