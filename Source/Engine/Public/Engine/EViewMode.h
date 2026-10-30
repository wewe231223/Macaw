/**
 * @file EViewMode.h
 * @brief 장면 전체의 채우기 방식을 선택합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

namespace Engine {

    /**
     * @brief 장면 전체에 적용하는 메시 채우기 방식입니다.
     */
    enum class EViewMode : Core::Uint8 {
        /**
         * @brief 재질과 기본 조명을 사용하여 면을 채웁니다.
         */
        Lit,
        /**
         * @brief 삼각형의 변을 와이어프레임으로 표시합니다.
         */
        Wireframe,
        /**
         * @brief 정점에서 계산한 조명을 보간하는 Gouraud 모드입니다.
         */
        Lit_Gouraud,
        /**
         * @brief 픽셀별 Lambert 확산 조명 모드입니다.
         */
        Lit_Lambert,
        /**
         * @brief 픽셀별 확산·정반사 조명 모드이며 Lit과 같은 값입니다.
         */
        Lit_Phong = Lit,
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
        SceneDepth = 6
    };

    /**
     * @brief 해당 열거형의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectEnum(EViewMode, Lit_Phong, Lit_Gouraud, Lit_Lambert, Unlit, WorldNormal, SceneDepth, Wireframe);

    /**
     * @fn UEnum* GetNativeEnum(TNativeEnum<EViewMode>)
     * @brief 타입 태그로 표시 모드의 열거형 메타데이터를 조회합니다.
     * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
     */

}
