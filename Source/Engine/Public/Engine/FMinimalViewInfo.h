/**
 * @file FMinimalViewInfo.h
 * @brief 카메라 관리자가 계산하고 블렌드하는 카메라 시점 값입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Math/FQuat.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector2.h"

namespace Engine {

    struct FSceneView;

    /**
     * @brief 한 프레임의 카메라 위치·회전·시야각입니다.
     * @ingroup Engine
     */
    struct FMinimalViewInfo {
    public:
        /**
         * @brief 두 시점 사이를 보간합니다.
         * @param[in] Other Alpha가 1일 때의 시점입니다.
         * @param[in] Alpha 0에서 1 사이의 보간 비율입니다.
         * @return 위치·시야각은 선형, 회전은 구면 선형 보간한 시점입니다.
         */
        Engine::FMinimalViewInfo Blend(const Engine::FMinimalViewInfo& Other, float Alpha) const;

        /**
         * @brief 뷰포트 크기에 맞는 뷰·투영 행렬을 계산합니다.
         * @param[in] Size 계산할 뷰포트의 가로·세로 픽셀 크기입니다.
         * @param[out] View 계산한 카메라 뷰·투영 상태를 받을 출력입니다.
         * @return 크기와 시점 값이 유효해 계산했으면 true입니다.
         */
        bool CalcSceneView(const Core::FVector2& Size, Engine::FSceneView& View) const;

    public:
        /**
         * @brief 카메라의 월드 위치입니다.
         */
        Core::FVector mLocation{};

        /**
         * @brief 카메라의 월드 회전입니다.
         */
        Core::FQuat mRotation{};

        /**
         * @brief 수직 시야각이며 도 단위입니다.
         */
        float mFieldOfView{60.0f};
    };

}
