/**
 * @file SpriteGeometry.h
 * @brief 렌더링·선택·경계 계산이 공유하는 스프라이트 정렬 행렬을 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Math/FMatrix.h"

namespace Engine {

    struct FDrawSprite;
    struct FSceneView;

    /**
     * @brief 렌더링과 피킹이 공유하는 쿼드의 로컬 XY에서 월드로의 행렬을 계산합니다.
     * @param[in] World 컴포넌트의 월드 변환입니다.
     * @param[in] Sprite 쿼드 크기와 카메라 정렬 여부입니다.
     * @param[in] View 카메라의 뷰 행렬입니다.
     * @return 스케일과 정렬을 적용한 쿼드의 월드 행렬입니다.
     */
    Core::FMatrix MakeSpriteWorld(const Core::FMatrix& World, const Engine::FDrawSprite& Sprite, const Engine::FSceneView& View);

}
